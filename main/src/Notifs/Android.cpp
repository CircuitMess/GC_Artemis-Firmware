#include "Android.h"
#include "Util/Services.h"
#include "Services/Time.h"
#include <esp_log.h>
#include <cctype>

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
	requestTime(); // request time after connection
}

void Android::onDisconnect(){
	if(!connected) return;
	connected = false;
	currentRingingState = false;
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
	const auto split_line = splitProtocolMsg(line);
	const auto& command = split_line[0];

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
	const auto&  protocolVersion = split_line[1];
	uart.printf("version;%s;%s\n", protocolVersion.c_str(), FirmwareVersion); // response, give protocol version even if missmatch
	if(protocolVersion != ProtocolVersion){
		ESP_LOGW(TAG, "Connection failed! Protocol version mismatch: version %s, expected %s", protocolVersion.c_str(), ProtocolVersion);
	}else{
		ESP_LOGI(TAG, "Connected! Hello received, protocol version: %s", protocolVersion.c_str());
		onConnect();
	}
}


// notifAdd;<notifID>;<title>;<content>;<appID>;<sender>;<category>;<labelPos>;<labelNeg>
void Android::handleNotifAdd(const std::vector<std::string>& split_line){
	const uint32_t id = std::stoull(split_line[1]);
	const uint32_t cat_val = !split_line[6].empty() ? std::stoull(split_line[6]) : 0;

	const Notif notif = {
			.uid = id,
			.title = split_line[2],
			.message = split_line[3],
			.appID = split_line[4],
			.category = mapNotifCategories(cat_val),
	};
	ESP_LOGI(TAG, "New notif ID %ld", notif.uid);

	notifNew(notif);
}

//notifDel;<notifID>
void Android::handleNotifDel(const std::vector<std::string>& split_line){
	const uint32_t id = std::stoull(split_line[1]);
	ESP_LOGI(TAG, "Del notif ID %ld", id);
	notifRemove(id);
}

// notifModify;<notifID>;<title>;<content>;<appID>;<sender>;<category>;<labelPos>;<labelNeg>
void Android::handleNotifModify(const std::vector<std::string>& split_line){
	const uint32_t id = std::stoull(split_line[1]);
	const uint32_t cat_val = !split_line[6].empty() ? std::stoull(split_line[6]) : 0;

	const Notif notif = {
			.uid = id,
			.title = split_line[2],
			.message = split_line[3],
			.appID = split_line[4],
			.category = mapNotifCategories(cat_val),
	};

	ESP_LOGI(TAG, "Mod notif ID %ld", notif.uid);
	notifModify(notif);
}

// callIncoming;<callID>;<callerName>;<callerNumber>
void Android::handleCallIncoming(const std::vector<std::string>& split_line){
	const uint32_t id = std::stoull(split_line[1]);
	const auto& name = split_line[2];
	const auto& number = split_line[3];

	if(currentRingingState) return;

	currentRingingState = true;

	const Notif notif = {
			.uid = (uint32_t) id,
			.title = name + " (" + number + ")", // name(number)
			.message = "Incoming call",
			.appID = "",
			.category = Notif::Category::IncomingCall
	};

	notifNew(notif);
}

//time;<timestamp>;<timezoneOffset>
void Android::handleTime(const std::vector<std::string>& split_line){
	const uint64_t timestamp = std::stoll(split_line[1]);
	const uint32_t timezone_offset = std::stoul(split_line[2]);
	ESP_LOGI(TAG, "Got UNIX time: %lld", timestamp);
	ESP_LOGI(TAG, "Got timezone: %ld", timezone_offset);

	if(timestamp == 0) return;

	auto time = timestamp + timezone_offset * 60;

	auto ts = static_cast<Time*>(Services.get(Service::Time));
	ts->setTime((time_t) time);
}

// callIncomingStop;<callID>
void Android::handleCallIncomingStop(const std::vector<std::string>& split_line){
	const uint32_t id = std::stoull(split_line[1]);

	ESP_LOGI(TAG, "Incoming call stopped for ID %ld", id);
	notifRemove(id);

	currentRingingState = false;
}

// findPhoneStopAck
void Android::handleFindPhoneStopAck(const std::vector<std::string>& split_line){
	ESP_LOGI(TAG, "Find phone stopped ack received"); // one-minute ringing timeout from app
}

void Android::requestTime(){
	if(!connected) return;
	uart.printf("time\n");
}

std::vector<std::string> Android::splitProtocolMsg(const std::string& s, char delim){
	std::vector<std::string> out;

	size_t i = 0;
	const size_t n = s.size();

	while(i < n){
		if(s[i] == delim){
			out.emplace_back("");
			++i;
			continue;
		}

		size_t numStart = i;
		int value = 0;
		bool isNumber = false;

		while(i < n && s[i] >= '0' && s[i] <= '9'){
			isNumber = true;
			value = value * 10 + (s[i] - '0');
			++i;
		}

		if(isNumber && i < n && s[i] == ':'){
			++i;

			out.emplace_back(s.substr(i, value));
			i += value;

			if(i < n && s[i] == delim){
				++i;
			}
		}else{
			i = numStart;
			size_t start = i;

			while(i < n && s[i] != delim){
				++i;
			}

			out.emplace_back(s.substr(start, i - start));

			if(i < n && s[i] == delim){
				++i;
			}
		}
	}

	if (!s.empty() && s.back() == delim){
        out.emplace_back("");
    }

	return out;
}


void Android::notifList(){
	if(!connected) return;
	uart.printf("notifList\n");
}

void Android::callReject(uint32_t uid){
	if(!connected) return;
	uart.printf("callReject;%d\n", uid);
}

Notif::Category Android::mapNotifCategories(const uint32_t category_val){
	static const std::unordered_map<uint32_t, Notif::Category> categoryMap = {
			{ 0, Notif::Category::Other },
			{ 1, Notif::Category::Social },
			{ 2, Notif::Category::Social },
			{ 3, Notif::Category::Schedule },
			{ 4, Notif::Category::MissedCall },
			{ 5, Notif::Category::News },
			{ 6, Notif::Category::Location },
			{ 7, Notif::Category::Entertainment }
	};

	if(!categoryMap.contains(category_val)){
		ESP_LOGW(TAG, "Unknown category value from app: %ld, defaulting to Other", category_val);
		return Notif::Category::Other;
	}

	return categoryMap.at(category_val);
}
