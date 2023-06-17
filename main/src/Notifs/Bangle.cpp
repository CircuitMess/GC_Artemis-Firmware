#include "Bangle.h"
#include <mjson.h>
#include <esp_log.h>
#include <cmath>

static const char* TAG = "Bangle";

Bangle::Bangle(BLE::Server* server) : Threaded("Bangle", 12 * 1024), server(server), uart(server){
	server->setOnConnectCb([this](const esp_bd_addr_t addr){ connect(); });
	server->setOnDisconnectCb([this](const esp_bd_addr_t addr){ disconnect(); });
	start();
}

Bangle::~Bangle(){
	stop();
	server->setOnConnectCb({});
	server->setOnDisconnectCb({});
}

void Bangle::actionPos(uint32_t uid){
	// TODO: pos & neg for call
}

void Bangle::actionNeg(uint32_t uid){
	// TODO: pos & neg for call
	uart.printf("{ t: \"notify\", id: %d, n: \"DISMISS\" }\n");
}

void Bangle::loop(){
	auto data = uart.scan_nl(portMAX_DELAY);
	if(!data || data->empty()) return;

	std::string line(data->cbegin(), data->cend());
	data.release();

	// trimming
	line.erase(line.begin(), std::find_if(line.begin(), line.end(), [](unsigned char ch) { return !std::isspace(ch); }));
	line.erase(std::find_if(line.rbegin(), line.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), line.end());

	ESP_LOGV(TAG, "%s", line.c_str());

	auto gbStart = line.find("GB(");
	if(gbStart != std::string::npos && line.back() == ')'){
		line.erase(line.cbegin(), line.cbegin() + gbStart + 3);
		line.pop_back();

		if(line[0] != '{' || line[line.size()-1] != '}'){
			ESP_LOGD(TAG, "Malformed JSON: %s", line.c_str());
			return;
		}

		handleCommand(line);
		return;
	}

	auto findArg = [&line](std::string fn){
		fn.push_back('(');

		auto fnStart = line.find(fn);
		if(fnStart == std::string::npos) return std::string();

		fnStart += fn.size();

		auto fnEnd = line.find(")", fnStart);
		if(fnEnd == std::string::npos) return std::string();

		return std::string(line.cbegin() + fnStart, line.cend() + fnEnd);
	};

	auto time = findArg("setTime");
	if(!time.empty()){
		auto unix = std::stoll(time);
		ESP_LOGI(TAG, "Got UNIX time: %lld", unix);
		// TODO: set time
	}

	auto timeZone = findArg("setTimeZone");
	if(!timeZone.empty()){
		auto offset = std::stod(timeZone);
		ESP_LOGI(TAG, "Got timezone: %f", offset);
		// TODO: set time zone
	}
}

void Bangle::handleCommand(const std::string& line){
	int comlen;
	const char* com;
	if(mjson_find(line.c_str(), line.size(), "$.t", &com, &comlen) != MJSON_TOK_STRING){
		ESP_LOGW(TAG, "Invalid JSON, missing command: %s", line.c_str());
		return;
	}

	std::string t(com + 1, com + comlen - 1);

	static const std::unordered_map<std::string, std::function<void(const std::string& line)>> handlers = {
			{ "is_gps_active", [this](const std::string& line){ handle_isGpsActive(); }},
			{ "find",          [this](const std::string& line){
				int on;
				int res = mjson_get_bool(line.c_str(), line.size(), "$.n", &on);
				handle_find(res && on);
			} },
			{ "notify",          [this](const std::string& line){ handle_notify(line); } },
			{ "notify-",         [this](const std::string& line){
				double id;
				int res = mjson_get_number(line.c_str(), line.size(), "$.id", &id);
				if(!res){
					ESP_LOGE(TAG, "Received notify del withoud id");
					return;
				}

				if(std::round(id) != id || id < 0){
					ESP_LOGE(TAG, "Received notify del command with invalid id: %f", id);
					return;
				}

				handle_notifyDel(id);
			} },
	};

	auto handler = handlers.find(t);
	if(handler == handlers.end()){
		ESP_LOGW(TAG, "Unhandled command from phone: %s", t.data());
		return;
	}

	ESP_LOGI(TAG, "Command: %s", t.data());
	handler->second(line);
}

void Bangle::handle_isGpsActive(){
	uart.printf("{ t: \"gps_power\", status: false }\n");
}

void Bangle::handle_find(bool on){
	ESP_LOGI(TAG, "Find watch: %d", on);
	// TODO: trigger an alarm or something
}

void Bangle::handle_notify(const std::string& line){
	auto get = [&line](std::string prop){
		prop.insert(0, "$.");
		int len;
		const char* val;
		if(mjson_find(line.c_str(), line.size(), prop.c_str(), &val, &len) != MJSON_TOK_STRING){
			ESP_LOGD(TAG, "Missing prop in notif: %s", prop.c_str() + 2);
			return std::string();
		}
		return std::string(val, val + len);
	};

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
			.title = get("title"),
			.subtitle = get("subject"),
			.message = get("body"),
			.appID = get("src")
	};

	ESP_LOGI(TAG, "New notif ID %ld", notif.uid);

	notifNew(notif);
}

void Bangle::handle_notifyDel(uint32_t id){
	ESP_LOGI(TAG, "Del notif ID %ld", id);
	notifRemove(id);
}