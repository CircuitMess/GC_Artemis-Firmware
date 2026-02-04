#include "Android.h"
#include "Util/Services.h"
#include "Services/Time.h"
#include <esp_log.h>

static const char* TAG = "Android";

Android::Android(BLE::Server* server) : Threaded("Android", 4 * 1024), server(server), uart(server){
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
	uart.printf("version;%s;%s\n", ProtocolVersion, FirmwareVersion); // response
	requestTime(); // request time after connection
}

void Android::onDisconnect(){
	if(!connected) return;
	connected = false;
	currentRingingState = false;
	currentCallId = -1;
	disconnect();
}

// notifPos;<notifID>
void Android::actionPos(uint32_t uid){
	if(!connected) return;

	uart.printf("notifPos;%d\n", uid);
}

// notifNeg;<notifID>
void Android::actionNeg(uint32_t uid){
	if(!connected) return;

	if(uid == currentCallId) return;

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
	auto split_line = splitProtocolMsg(line);
	auto command = split_line[0];

	if(command == "hello"){
		if(split_line.size() < 2){
		ESP_LOGW(TAG, "Invalid hello command");
		return;
		}

		handleHello(split_line);
		return;
	}

	if(!connected) return; // ignore all commmands until connected

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
	}else if(command == "callIncomingStop"){
		if(split_line.size() < 2){
			ESP_LOGW(TAG, "Invalid callIncomingStop command: %s", line.c_str());
			return;
		}

		handleCallIncomingStop(split_line);
		return;
	}else if(command == "findPhoneStopAck"){
		handleFindPhoneStopAck(split_line);
		return;
	}else{
		ESP_LOGW(TAG, "Unknown command: %s", line.c_str());
		return;
	}
}

// hello;<protocolVersion>
void Android::handleHello(const std::vector<std::string>& split_line){
	auto protocolVersion = split_line[1];

	if(protocolVersion != ProtocolVersion) {
		ESP_LOGW(TAG, "Connection failed! Protocol version mismatch: version %s, expected %s", protocolVersion.c_str(), ProtocolVersion);
	}
	else{
		ESP_LOGI(TAG, "Connected! Hello received, protocol version: %s", protocolVersion.c_str());
		onConnect();
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
	uint32_t id =  std::stoll(split_line[1]); // notif currently supports only uint32_t, don't use example from protocol docs!
	auto name = split_line[2];
	auto number = split_line[3];

	if(currentCallId == -1 && currentRingingState == false){
		currentCallId = id;
		currentRingingState = true;
	}else{
		ESP_LOGI(TAG, "Already in call, ignoring incoming call from %s (%s)", name.c_str(), number.c_str());
		return;
	} 
	
	Notif notif = {
			.uid = (uint32_t) id,
			.title = name + " (" + number + ")", // name(number)
			.message = "Incoming call",
			.appID = "",
			.category = Notif::Category::IncomingCall
	};

	// notifModify(notif);
	notifNew(notif);
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
void Android::handleCallIncomingStop(const std::vector<std::string>& split_line){
	uint32_t id = std::stoll(split_line[1]);

	if(currentCallId != id || currentRingingState == false) return; // ignore

	ESP_LOGI(TAG, "Incoming call stopped for ID %ld", currentCallId);
	notifRemove(currentCallId);

	currentCallId = -1;
	currentRingingState = false;
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
