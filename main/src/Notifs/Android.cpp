#include "Android.h"
#include "Util/Services.h"
#include "Services/Time.h"
#include <esp_log.h>
#include <cctype>
#include <optional>
#include <algorithm>

static const char* TAG = "Android";

Android::Android(BLE::Server* server) : Threaded("Android", 4 * 1024), server(server), uart(server){
	callIds.reserve(4);
	rxBuf.reserve(1024);
	disconnectSub = server->addOnDisconnectCb([this](const esp_bd_addr_t addr){ onDisconnect(); });
	start();
}

Android::~Android(){
	stop();
	server->removeOnDisconnectCb(disconnectSub);
}

// Walks `buf` honoring length-prefixed string parameters (`<length>:<bytes>`), so a
// `\n` inside such a string does not falsely end the frame. Returns the iterator one
// past the terminating top-level `\n` (may equal buf.cend()), or std::nullopt when
// more data is needed.
static std::optional<PSRAMByteBuffer::const_iterator> findFrameEnd(const PSRAMByteBuffer& buf){
	auto it = buf.cbegin();
	while(true){
		auto p = it;
		size_t value = 0;
		bool isNumber = false;
		while(p != buf.cend() && *p >= '0' && *p <= '9'){
			isNumber = true;
			value = value * 10 + (*p - '0');
			++p;
		}

		if(isNumber && p != buf.cend() && *p == ':'){
			++p;
			if((size_t) std::distance(p, buf.cend()) < value) return std::nullopt;
			it = p + value;
		}else{
			while(it != buf.cend() && *it != ';' && *it != '\n') ++it;
			if(it == buf.cend()) return std::nullopt;
		}

		if(it == buf.cend()) return std::nullopt;
		if(*it == '\n') return it + 1;
		// *it == ';'
		++it;
	}
}

void Android::onConnect(){
	if(connected) return;
	connected = true;
	NotifSource::connect();
	MediaSource::connect();

	uart.printf("time\n");
	uart.printf("notifList\n");
}

void Android::onDisconnect(){
	if(!connected) return;
	connected = false;
	findPhone = false;
	callIds.clear();
	rxBuf.clear();
	NotifSource::disconnect();
	MediaSource::disconnect();
}

// notifPos;<notifID>
void Android::actionPos(uint32_t uid){
	if(!connected) return;

	uart.printf("notifPos;%d\n", uid);
}

// notifNeg;<notifID>
void Android::actionNeg(uint32_t uid){
	if(!connected) return;

	if(callIds.contains(uid)){
		uart.printf("callReject;%d\n", uid);
		callIds.erase(uid);
		notifRemove(uid);
		return;
	}

	uart.printf("notifNeg;%d\n", uid);
}

void Android::findPhoneStart(){
	if(!connected) return;
	if(findPhone) return;
	uart.printf("findPhoneStart\n");
	findPhone = true;
}

void Android::findPhoneStop(){
	if(!connected) return;
	if(!findPhone) return;
	uart.printf("findPhoneStop\n");
	findPhone = false;
}

bool Android::findPhoneActive(){
	return findPhone;
}

void Android::loop(){
	auto chunk = uart.scan_nl(portMAX_DELAY);
	if(!chunk || chunk->empty()) return;

	rxBuf.insert(rxBuf.end(), chunk->cbegin(), chunk->cend());
	chunk.reset();

	while(true){
		auto frameEnd = findFrameEnd(rxBuf);
		if(!frameEnd) break;

		std::string line(rxBuf.cbegin(), *frameEnd);
		rxBuf.erase(rxBuf.cbegin(), *frameEnd);

		// trimming
		line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch){ return !std::isspace(ch); }));
		line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch){ return !std::isspace(ch); }).base(), line.end());

		handleCommand(line);
	}
}

void Android::handleCommand(const std::string& line){
	const auto split_line = splitProtocolMsg(line);
	if(split_line.empty()) return;
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
		if(split_line.size() < 9){
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
		if(split_line.size() < 7){
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
		handleFindPhoneStopAck();
		return;
	}else if(command == "findPhoneStopNack"){
		handleFindPhoneStopNack();
		return;
	}else if(command == "mediaState"){
		if(split_line.size() < 2){
			ESP_LOGW(TAG, "Invalid mediaState command: %s", line.c_str());
			return;
		}

		handleMediaState(split_line);
		return;
	}else if(command == "mediaInfo"){
		if(split_line.size() < 5){
			ESP_LOGW(TAG, "Invalid mediaInfo command: %s", line.c_str());
			return;
		}

		handleMediaInfo(split_line);
		return;
	}else{
		ESP_LOGW(TAG, "Unknown command: %s", line.c_str());
		return;
	}
}

// hello;<protocolVersion>
void Android::handleHello(const std::vector<std::string>& split_line){
	const auto&  protocolVersion = split_line[1];
	uart.printf("version;%s;%s\n", ProtocolVersion, FirmwareVersion);
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
	ESP_LOGI(TAG, "New notif ID %ld, cat %s, notifPos: %s, notifNeg: %s", notif.uid, split_line[6].c_str(), split_line[7].c_str(), split_line[8].c_str());

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

	const Notif notif = {
			.uid = (uint32_t) id,
			.title = name,
			.message = number,
			.category = Notif::Category::IncomingCall
	};

	callIds.insert(id);
	notifNew(notif);
}

//time;<timestamp>;<timezoneOffset>
void Android::handleTime(const std::vector<std::string>& split_line){
	const int64_t timestamp = std::stoll(split_line[1]);
	const int32_t timezone_offset = std::stol(split_line[2]); // signed: negative offsets like -480 (UTC-8) are valid
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

	callIds.erase(id);
	notifRemove(id);
}

// findPhoneStopAck
void Android::handleFindPhoneStopAck(){
	ESP_LOGI(TAG, "Find phone stopped ack received"); // one-minute ringing timeout from app
	findPhone = false;
}

// findPhoneStopNack
void Android::handleFindPhoneStopNack(){
	ESP_LOGI(TAG, "Find phone stopped nack received");
	findPhone = false;
}

// mediaState;<state>
void Android::handleMediaState(const std::vector<std::string>& split_line){
	const uint8_t state_val = std::stoul(split_line[1]);
	
	MediaState media_state = MediaState::Stopped;

	if(state_val == 0) media_state = MediaState::Stopped;
	else if(state_val == 1) media_state = MediaState::Playing;
	else if(state_val == 2) media_state = MediaState::Paused;
	else ESP_LOGW(TAG, "Unknown media state value from app: %d, defaulting to Stopped", state_val);
	
	ESP_LOGI(TAG, "Media state changed to: %d", state_val);
	mediaState(media_state);
}

// mediaInfo;<title>;<artist>;<album>;<appID>
void Android::handleMediaInfo(const std::vector<std::string>& split_line){
	const auto& title = split_line[1];
	const auto& artist = split_line[2];
	const auto& album = split_line[3];
	const auto& appID = split_line[4];
	
	const MediaInfo media = {
		.title = title,
		.artist = artist,
		.album = album,
		.appID = appID
	};
	
	ESP_LOGI(TAG, "Media info: title=%s, artist=%s, album=%s, appID=%s", title.c_str(), artist.c_str(), album.c_str(), appID.c_str());
	mediaInfo(media);
}

// mediaPlay
void Android::mediaPlay(){
	if(!connected) return;
	uart.printf("mediaPlay\n");
}

// mediaPause
void Android::mediaPause(){
	if(!connected) return;
	uart.printf("mediaPause\n");
}

// mediaNext
void Android::mediaNext(){
	if(!connected) return;
	uart.printf("mediaNext\n");
}

// mediaPrev
void Android::mediaPrev(){
	if(!connected) return;
	uart.printf("mediaPrev\n");
}

std::vector<std::string> Android::splitProtocolMsg(const std::string& s, char delim){
	std::vector<std::string> out;
	out.reserve(static_cast<size_t>(std::count(s.begin(), s.end(), delim)) + 1);

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
