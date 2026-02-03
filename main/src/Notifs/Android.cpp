#include "Android.h"
#include "Util/Services.h"
#include "Services/Time.h"
#include <esp_log.h>

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

Android::Android(BLE::Server* server) : Threaded("Android", 4 * 1024), server(server), uart(server){
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
	ESP_LOGI(TAG, "Sent from app: hello;1\n"); // mimic hello;<protocolVersion> from app
	uart.printf("version;%s;%s\n", ProtocolVersion, FirmwareVersion); // response
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

// notifPos;<notifID>
void Android::actionPos(uint32_t uid){
	if(!connected) return;

	uart.printf("notifPos;%d\n", uid);
	// TODO: pos & neg for call
}

// notifNeg;<notifID>
void Android::actionNeg(uint32_t uid){
	if(!connected) return;
	// TODO: pos & neg for call

	if(uid == currentCallId) return;

	if(missedCalls.count(uid)){
		missedCalls.erase(uid);
		return;
	}

	uart.printf("notifNeg;%d\n", uid);
}

void Android::findPhoneStart(){
	if(!connected) return;
	uart.printf("findPhoneStart\n");
}

void Android::findPhoneStop(){
	if(!connected) return;
	uart.printf("findPhoneStop\n");
}

void Android::loop(){
	auto data = uart.scan_nl(portMAX_DELAY);
	if(!data || data->empty()) return;

	std::string line(data->cbegin(), data->cend());
	data.reset();

	// trimming
	line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch){ return !std::isspace(ch); }));
	line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());

	handleCommand(line);
}

void Android::handleCommand(const std::string& line){
	if(!connected) return;

	auto split_line = splitProtocolMsg(line);
	auto command = split_line[0];

	if(command == "time"){
		if(split_line.size() < 3){
			ESP_LOGW(TAG, "Invalid time command: %s", line.c_str());
			return;
		}

		handleTime(split_line);
		return;
	}else if(command == "notifAdd"){
		if(split_line.size() < 5){
			ESP_LOGW(TAG, "Invalid notifAdd command: %s", line.c_str());
			return;
		}

		handleNotifAdd(split_line);
		return;
	}else if(command == "notifDel"){
		if(split_line.size() < 2){
			ESP_LOGW(TAG, "Invalid notifDel command: %s", line.c_str());
			return;
		}

		handleNotifDel(split_line);
		return;
	}else if(command == "notifModify"){
		if(split_line.size() < 5){
			ESP_LOGW(TAG, "Invalid notifMod command: %s", line.c_str());
			return;
		}

		handleNotifModify(split_line);
		return;
	}else if(command == "callIncoming"){
		if(split_line.size() < 4){
			ESP_LOGW(TAG, "Invalid callIncoming command: %s", line.c_str());
			return;
		}

		handleCallIncoming(split_line);
		return;
	}else if(command == "incomingStop"){
		if(split_line.size() < 2){
			ESP_LOGW(TAG, "Invalid incomingStop command: %s", line.c_str());
			return;
		}

		handleIncomingStop(split_line);
		return;
	}else if(command == "findPhoneStopAck"){
		handleFindPhoneStopAck(split_line);
		return;
	}else{
		ESP_LOGW(TAG, "Unknown command: %s", line.c_str());
		return;
	}
}

// notifAdd;<notifID>;<title>;<content>;<appID>;<sender>;<category>;<labelPos>;<labelNeg>
void Android::handleNotifAdd(const std::vector<std::string>& split_line){
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

//notifDel;<notifID>
void Android::handleNotifDel(const std::vector<std::string>& split_line){
	uint32_t id = std::stoul(split_line[1]);
	ESP_LOGI(TAG, "Del notif ID %ld", id);
	notifRemove(id);
}

// notifModify;<notifID>;<title>;<content>;<appID>;<sender>;<category>;<labelPos>;<labelNeg>
void Android::handleNotifModify(const std::vector<std::string>& split_line){
	Notif notif = {
			.uid = std::stoul(split_line[1]),
			.title = split_line[2],
			.message = split_line[3],
			.appID = split_line[4],
			.category = Notif::Category::Other, // TODO: map category and other optional
	};

	ESP_LOGI(TAG, "Mod notif ID %ld", notif.uid);
	notifModify(notif);
}

// callIncoming;<callID>;<callerName>;<callerNumber>
void Android::handleCallIncoming(const std::vector<std::string>& split_line){
	const auto hash = [](const std::string& str){
		uint32_t n = 0;
		for(int i = 0; i < str.size(); i++){
			n += str.at(i) * i;
		}
		return n;
	};

	auto name = split_line[2];
	auto number = split_line[3];
	auto uid = hash(name) * hash(number);

	CallCmd command = CallCmd::Incoming;

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
			.title = name + " (" + number + ")", // name(number)
			.message = info.message, //incoming call, missed call
			.appID = "",
			.category = info.category
	};

	notifModify(notif);
}

//time;<timestamp>;<timezoneOffset>
void Android::handleTime(const std::vector<std::string>& split_line){
	uint64_t timestamp = std::stoll(split_line[1]);
	float timezone_offset = std::stod(split_line[2]);
	ESP_LOGI(TAG, "Got UNIX time: %lld", timestamp);
	ESP_LOGI(TAG, "Got timezone: %f", timezone_offset);

	if(timestamp == 0) return;

	auto time = timestamp + timezone_offset * 60;

	auto ts = static_cast<Time*>(Services.get(Service::Time));
	ts->setTime((time_t) time);

	// If we receive time from the device, we'll consider this the "connected" event. Might happen
	// multiple times during session, but since we're already connected, those onConnect calls will
	// be discarded. Main thing is, we're sure we'll get the time first thing when connected
	onConnect();
}

// callIncomingStop;<callID>
void Android::handleIncomingStop(const std::vector<std::string>& split_line){
	uint32_t id = std::stoul(split_line[1]);
	if(currentCallId != id) return;

	if(currentCallState == CallState::Incoming){
		currentCallState = CallState::IncomingMissed;
		missedCalls.insert(id);
		ESP_LOGI(TAG, "Call ID %ld marked as missed", id);
	}else{
		currentCallState = CallState::None;
		notifRemove(id);
		ESP_LOGI(TAG, "Call ID %ld ended", id);
	}
}

// findPhoneStopAck
void Android::handleFindPhoneStopAck(const std::vector<std::string>& split_line){
	ESP_LOGI(TAG, "Find phone stopped ack received"); // one-minute ringing timeout from app
}

void Android::requestTime(){
	if(!connected) return;
	uart.printf("time\n"); // TODO: handle after app implementation
}

std::vector<std::string> Android::splitProtocolMsg(const std::string& s, char delim){
    std::vector<std::string> out;

    size_t start = 0;

    while(true){
        size_t pos = s.find(delim, start);

        if(pos == std::string::npos){
            out.emplace_back(s.substr(start));
            break;
        }

        out.emplace_back(s.substr(start, pos - start));
        start = pos + 1;
    }

    return out;
}

void Android::notifList(){
	if(!connected) return;
	uart.printf("notifList\n"); // TODO: handle after app implementation
}

void Android::callReject(uint32_t uid){
	if(!connected) return;
	uart.printf("callReject;%d\n", uid);
	// TODO: handle after app implementation
}
