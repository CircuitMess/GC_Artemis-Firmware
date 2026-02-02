#include "Android.h"
#include <esp_log.h>
#include <mjson.h>
#include <cmath>
#include <regex>
#include <mbedtls/base64.h>
#include "Util/Services.h"
#include "Services/Time.h"

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
	printf("Sent from app: hello;1\n"); // mimic hello;<protocolVersion> from app
	// TODO: verify protocol version - display "Outdated firmware"
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

	handleCommand(line);
}

void Android::handleCommand(const std::string& line){
	if(!connected) return;

	auto split_line = splitProtocolMsg(line);
	auto command = split_line[0];

	//time;<timestamp>;<timezoneOffset>
	if (command == "time"){
		if (split_line.size() < 3){
			ESP_LOGW(TAG, "Invalid time command: %s", line.c_str());
			return;
		}

		timestamp = std::stoll(split_line[1]);
		timezone_offset = std::stod(split_line[2]);
		ESP_LOGI(TAG, "Got UNIX time: %lld", timestamp);
		ESP_LOGI(TAG, "Got timezone: %f", timezone_offset);
		setTime();
		return;
	}

	// notifAdd;<notifID>;<title>;<content>;<appID>;<sender>;<category>;<labelPos>;<labelNeg>
	if(command == "notifAdd"){
		handleAddNotify(split_line);
		return;
	}
}

void Android::handleAddNotify(const std::vector<std::string> split_line){
	// 5 required params + 4 optional
	if(split_line.size() < 5){
		ESP_LOGW(TAG, "Invalid notifAdd command: insufficient parameters!");
		return;
	}

	uint32_t id = std::stoul(split_line[1]);

	Notif notif = {
			.uid = id,
			.title = split_line[2],
			.message = split_line[3],
			.appID = split_line[4],
			.category = Notif::Category::Other, // TODO: map category and other optional
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

void Android::setTime(){
	if(timestamp == 0) return;

	auto time = timestamp + timezone_offset * 60;

	auto ts = static_cast<Time*>(Services.get(Service::Time));
	ts->setTime((time_t) time);

	// If we receive time from the device, we'll consider this the "connected" event. Might happen
	// multiple times during session, but since we're already connected, those onConnect calls will
	// be discarded. Main thing is, we're sure we'll get the time first thing when connected
	onConnect();
}


std::vector<std::string> Android::splitProtocolMsg(const std::string& s, char delim){
    std::vector<std::string> out;

    size_t start = 0;

    while (true) {
        size_t pos = s.find(delim, start);

        if (pos == std::string::npos) {
            out.emplace_back(s.substr(start));
            break;
        }

        out.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }

    return out;
}