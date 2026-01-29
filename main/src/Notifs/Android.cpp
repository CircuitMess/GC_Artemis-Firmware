#include "Android.h"
#include <esp_log.h>
#include <mjson.h>
#include <cmath>
#include <regex>
#include <mbedtls/base64.h>

static const char* TAG = "Android";

const std::map<std::pair<Android::CallState, Android::CallCmd>, Android::CallState> Android::CallTransitions = {
		{ { Android::CallState::None,             Android::CallCmd::Incoming }, Android::CallState::Incoming },
		{ { Android::CallState::None,             Android::CallCmd::Outgoing }, Android::CallState::Outgoing },
		{ { Android::CallState::Incoming,         Android::CallCmd::End },      Android::CallState::IncomingMissed },
		{ { Android::CallState::Incoming,         Android::CallCmd::Start },    Android::CallState::IncomingAccepted },
		{ { Android::CallState::IncomingMissed,   Android::CallCmd::Any },      Android::CallState::None },
		{ { Android::CallState::IncomingAccepted, Android::CallCmd::End },      Android::CallState::None },
		{ { Android::CallState::Outgoing,         Android::CallCmd::End },      Android::CallState::None }
};

const std::unordered_map<Android::CallState, Android::CallInfo> Android::CallInfoMap = {
		{ Android::CallState::Incoming,         { "Incoming call",       Notif::Category::IncomingCall } },
		{ Android::CallState::IncomingMissed,   { "Missed call",         Notif::Category::MissedCall } },
		{ Android::CallState::Outgoing,         { "Calling...",          Notif::Category::OutgoingCall } },
		{ Android::CallState::IncomingAccepted, { "Call in progress...", Notif::Category::IncomingCall } }
};

Android::Android(BLE::Server* server)
	: Threaded("Android", 4 * 1024),
	  server(server),
	  uart(server) {
	server->setOnConnectCb([this](const esp_bd_addr_t addr){ onConnect(); });
	server->setOnDisconnectCb([this](const esp_bd_addr_t addr){ onDisconnect(); });
	start();
}

Android::~Android(){
	stop();
	server->setOnConnectCb({});
	server->setOnDisconnectCb({});
}

void Android::onConnect(){
	if(connected) return;
	connected = true;
	connect();
}

void Android::onDisconnect(){
	if(!connected) return;
	connected = false;
	currentCallState = CallState::None;
	currentCallId = -1;
	missedCalls.clear();
	disconnect();
}

void Android::actionPos(uint32_t uid){
	if(!connected) return;
	// TODO: pos & neg for call
}

void Android::actionNeg(uint32_t uid){
	if(!connected) return;
	// TODO: pos & neg for call

	if(uid == currentCallId) return;

	if(missedCalls.count(uid)){
		missedCalls.erase(uid);
		return;
	}

	uart.printf("{t:\"notify\",id:%d,n:\"DISMISS\"} \n", uid);
}

void Android::findPhoneStart(){
	if(!connected) return;
	uart.printf("{t:\"findPhone\",n:true} \n");
}

void Android::findPhoneStop(){
	if(!connected) return;
	uart.printf("{t:\"findPhone\", n:false} \n");
}

void Android::loop(){
	printf("Waiting for data...\n");
	auto data = uart.scan_nl(portMAX_DELAY);
	if(!data || data->empty()) return;

	std::string line(data->cbegin(), data->cend());
	data.reset();

	// trimming
	line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch){ return !std::isspace(ch); }));
	line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());

	printf("RX line: %s", line.c_str()); // debugging
	// ESP_LOGI(TAG, "RX line: %s", line.c_str());

	if(line[0] != '{' || line[line.size() - 1] != '}'){
		ESP_LOGD(TAG, "Malformed JSON: %s", line.c_str());
		return;
	}

	handleCommand(line);
}

void Android::handleCommand(const std::string& line){
	if(!connected) return;

	int comlen;
	const char* com;
	if(mjson_find(line.c_str(), line.size(), "$.t", &com, &comlen) != MJSON_TOK_STRING){
		ESP_LOGW(TAG, "Invalid JSON, missing command: %s", line.c_str());
		return;
	}

	std::string t(com + 1, com + comlen - 1);

	static const std::unordered_map<std::string, std::function<void(const std::string& line)>> handlers = {
			{ "notify",        [this](const std::string& line){ handle_notify(line); } },
			{ "notify-",       [this](const std::string& line){
				double id;
				int res = mjson_get_number(line.c_str(), line.size(), "$.id", &id);
				if(!res){
					ESP_LOGE(TAG, "Received notify del without id");
					return;
				}

				if(std::round(id) != id || id < 0){
					ESP_LOGE(TAG, "Received notify del command with invalid id: %f", id);
					return;
				}

				handle_notifyDel(id);
			} },
			{ "call",          [this](const std::string& line){ handle_call(line); } }
	};

	auto handler = handlers.find(t);
	if(handler == handlers.end()){
		ESP_LOGW(TAG, "Unhandled command from phone: %s", t.data());
		return;
	}

	ESP_LOGI(TAG, "Command: %s", t.data());
	handler->second(line);
}

void Android::handle_notify(const std::string& line){
	double id;
	int res = mjson_get_number(line.c_str(), line.size(), "$.id", &id);
	if(!res){
		ESP_LOGE(TAG, "Received notify without id");
		return;
	}

	if(std::round(id) != id || id < 0){
		ESP_LOGE(TAG, "Received notify with invalid id: %f", id);
		return;
	}

	Notif notif = {
			.uid = (uint32_t) id,
			.title = getProperty(line, "title"),
			.message = getProperty(line, "body"),
			.appID = getProperty(line, "src"),
			.category = Notif::Category::Other,
	};

	ESP_LOGI(TAG, "New notif ID %ld", notif.uid);

	notifNew(notif);
}

void Android::handle_notifyDel(uint32_t id){
	ESP_LOGI(TAG, "Del notif ID %ld", id);
	notifRemove(id);
}

void Android::handle_call(const std::string& line){
	const auto hash = [](const std::string& str){
		uint32_t n = 0;
		for(int i = 0; i < str.size(); i++){
			n += str.at(i) * i;
		}
		return n;
	};

	auto name = getProperty(line, "name");
	auto number = getProperty(line, "number");
	auto uid = hash(name) * hash(number);

	auto cmd = getProperty(line, "cmd");
	CallCmd command = CallCmd::Invalid;
	if(cmd == "outgoing"){
		command = CallCmd::Outgoing;
	}else if(cmd == "end"){
		command = CallCmd::End;
	}else if(cmd == "incoming"){
		command = CallCmd::Incoming;
	}else if(cmd == "start"){
		command = CallCmd::Start;
	}else{
		ESP_LOGW(TAG, "Invalid call cmd: %s", cmd.c_str());
	}


	if(currentCallId == -1){
		currentCallId = uid;
		currentCallState = CallState::None;
	}

	//transition used only for incomingMissed -> None
	if(CallTransitions.count({ currentCallState, CallCmd::Any })){
		currentCallState = CallTransitions.at({ currentCallState, CallCmd::Any });
	}

	//take care of edge-cases when multiple simultaneous calls occur
	if(uid != currentCallId && currentCallState != CallState::None){

		const bool inCall = (currentCallState == CallState::IncomingAccepted || currentCallState == CallState::Outgoing);
		const bool inNewCall = (command == CallCmd::Start || command == CallCmd::Outgoing);
		const bool newCallRinging = (command == CallCmd::Incoming);
		const bool ringing = (currentCallState == CallState::Incoming);

		if(command == CallCmd::End && !missedCalls.count(currentCallId)){
			currentCallState = CallState::None;
			//in case of putting calls on hold, the end command can be sent with a different number, so terminate the call if it happens
			notifRemove(currentCallId);
			return;
		}

		if(inCall && newCallRinging){
			return;
		}else if(ringing && newCallRinging){
			notifRemove(currentCallId);
			currentCallState = CallState::None;
		}else if((ringing || inCall) && inNewCall){
			notifRemove(currentCallId);
			if(command == CallCmd::Start){
				currentCallState = CallState::Incoming;
			}else if(command == CallCmd::Outgoing){
				currentCallState = CallState::None;
			}
		}else{
			return; //treat all other cases as invalid, keep old call as the one "active"
		}
	}


	currentCallId = uid;

	if(CallTransitions.count({ currentCallState, command })){
		currentCallState = CallTransitions.at({ currentCallState, command });
	}else{
		currentCallState = CallState::None;
	}

	if(!CallInfoMap.count(currentCallState) && !missedCalls.count(uid)){
		notifRemove(uid);
		return;
	}

	const auto& info = CallInfoMap.at(currentCallState);

	Notif notif = {
			.uid = (uint32_t) uid,
			.title = name + " (" + number + ")", //ime(broj)
			.message = info.message, //incoming call, missed call
			.appID = "",
			.category = info.category
	};

	notifModify(notif);
}

std::string Android::getProperty(const std::string& line, std::string prop){
	prop.insert(0, "$.");
	int len;
	const char* val;
	std::string s;

	if(mjson_find(line.c_str(), line.size(), prop.c_str(), &val, &len) == MJSON_TOK_B64){
		s = std::string(val + 1, val + len - 1);

		size_t outLen = 0;
		auto ret = mbedtls_base64_decode(nullptr, 0, &outLen, (unsigned char*) s.c_str(), s.length());
		if(ret != MBEDTLS_ERR_BASE64_BUFFER_TOO_SMALL || outLen <= 0){
			ESP_LOGW(TAG, "(1) Failed decoding base64: %s | Return status: %d", s.c_str(), ret);
			return {};
		}

		std::string decoded(outLen + 1, '\0');
		ret = mbedtls_base64_decode((unsigned char*) decoded.data(), decoded.size(), &outLen, (unsigned char*) s.c_str(), s.length());
		if(ret != 0 || outLen <= 0){
			ESP_LOGW(TAG, "(2) Failed decoding base64: %s | Return status: %d", s.c_str(), ret);
			return {};
		}
		decoded.resize(outLen);

		s = std::move(decoded);
	}else if(mjson_find(line.c_str(), line.size(), prop.c_str(), &val, &len) == MJSON_TOK_STRING){
		s = std::string(val + 1, val + len - 1);
	}else{
		ESP_LOGD(TAG, "Missing prop in notif: %s", prop.c_str() + 2);
		return {};
	}

	s = std::regex_replace(s, std::regex(R"(\\u[a-zA-Z0-9]{3,4})"), "?");
	s = std::regex_replace(s, std::regex(R"(\\n)"), "\n");
	s = std::regex_replace(s, std::regex(R"(\\r)"), "\r");
	s = std::regex_replace(s, std::regex(R"(\\\\)"), "\\");
	s = std::regex_replace(s, std::regex(R"(\\t)"), "\t");
	s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
	std::replace(s.begin(), s.end(), '\t', ' ');

	return s;
}
