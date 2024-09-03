#include "WiFiSTA.h"
#include "Util/Events.h"
#include <esp_event.h>
#include <esp_netif.h>
#include <esp_wifi.h>
#include <esp_wifi_default.h>
#include <string>
#include <cstring>
#include <esp_log.h>

static const char* TAG = "WiFi_STA";

static std::string mac2str(uint8_t ar[]){
	std::string str;
	for(int i = 0; i < 6; ++i){
		char buf[3];
		sprintf(buf, "%02X", ar[i]);
		str += buf;
		if(i < 5) str += ':';
	}
	return str;
}

WiFiSTA::WiFiSTA() : hysteresis({0, 20, 40, 60, 80}, 1){
	ESP_ERROR_CHECK(esp_event_loop_create_default());

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_netif_init());
	netif = createNetif();

	ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_FLASH));
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

	esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, [](void* arg, esp_event_base_t base, int32_t id, void* data){
		if(base != WIFI_EVENT) return;
		auto wifi = static_cast<WiFiSTA*>(arg);
		wifi->event(id, data);
	}, this, &evtHandler);
}

WiFiSTA::~WiFiSTA(){
	ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &evtHandler));
	ESP_ERROR_CHECK(esp_event_loop_delete_default());

	esp_wifi_scan_stop();
	if(esp_wifi_disconnect() == ESP_FAIL){
		abort();
	}
	esp_err_t err = esp_wifi_stop();
	if (err == ESP_ERR_WIFI_NOT_INIT) {
		return;
	}
	ESP_ERROR_CHECK(err);
	ESP_ERROR_CHECK(esp_wifi_deinit());
	ESP_ERROR_CHECK(esp_wifi_clear_default_wifi_driver_and_handlers(netif));
	esp_netif_destroy(netif);
	netif = nullptr;
}

bool WiFiSTA::hasCachedSSID() const{
	return !cachedSSID.empty() && !attemptedCachedSSID;
}

void WiFiSTA::event(int32_t id, void* data){
	ESP_LOGE(TAG, "Evt %ld", id);

	if(id == WIFI_EVENT_STA_START){
		initSem.release();
	}else if(id == WIFI_EVENT_STA_CONNECTED){
		auto event = (wifi_event_sta_connected_t*) data;

		const auto mac = mac2str(event->bssid);
		ESP_LOGI(TAG, "connected bssid %s, chan=%d", mac.c_str(), event->channel);

		const esp_netif_ip_info_t ip = {
				.ip =		{ .addr = esp_ip4addr_aton("11.0.0.2") },
				.netmask =	{ .addr = esp_ip4addr_aton("255.255.255.0") },
				.gw =		{ .addr = esp_ip4addr_aton("11.0.0.1") },
		};
		const auto netif = esp_netif_get_default_netif();
		esp_netif_set_ip_info(netif, &ip);

		if(state == ConnAbort){
			esp_wifi_disconnect();
			return;
		}

		state = Connected;

		if(hasCachedSSID()){
			return;
		}

		Event evt { .action = Event::Connect, .connect = { .success = true } };
		memcpy(evt.connect.bssid, event->bssid, 6);
		Events::post(Facility::WiFiSTA, evt);

	}else if(id == WIFI_EVENT_STA_DISCONNECTED){
		auto event = (wifi_event_sta_disconnected_t*) data;
		const auto mac = mac2str(event->bssid);
		ESP_LOGI(TAG, "disconnected bssid %s, reason=%d", mac.c_str(), event->reason);

		if(hasCachedSSID() && event->reason == WIFI_REASON_SA_QUERY_TIMEOUT){
			Event evt{ .action = Event::Probe, .connect = { .success = false } };
			Events::post(Facility::WiFiSTA, evt);

			state = Disconnected;
			connect();
			return;
		}

		if(state == Connecting){
			if(++connectTries <= ConnectRetries){
				esp_wifi_connect();
			}else{
				state = Disconnected;
				Event evt { .action = Event::Connect, .connect = { .success = false } };
				Events::post(Facility::WiFiSTA, evt);
			}

			return;
		}

		if(state == ConnAbort){
			state = Disconnected;
			Event evt{ .action = Event::Connect, .connect = { .success = false } };
			Events::post(Facility::WiFiSTA, evt);
			return;
		}

		state = Disconnected;

		Event evt { .action = Event::Disconnect };
		memcpy(evt.disconnect.bssid, event->bssid, 6);
		Events::post(Facility::WiFiSTA, evt);
	}else if(id == WIFI_EVENT_SCAN_DONE){
		if(state == ConnAbort){
			state = Disconnected;
			Event evt{ .action = Event::Connect, .connect = { .success = false } };
			Events::post(Facility::WiFiSTA, evt);
			return;
		}else if(state != Scanning) return;

		if(attemptedCachedSSID){
			cachedSSID = "";
			attemptedCachedSSID = false;
		}

		std::string netSSID = cachedSSID;

		if(netSSID.empty()){
			wifi_ap_record_t ap_info[ScanListSize];
			memset(ap_info, 0, sizeof(ap_info));

			auto number = ScanListSize;
			ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&number, ap_info));

			auto ret = findNetwork(ap_info, number);

			if(ret == nullptr){
				state = Disconnected;
				Event evt{ .action = Event::Connect, .connect = { .success = false } };
				Events::post(Facility::WiFiSTA, evt);
				return;
			}

			netSSID = cachedSSID = std::string((const char*) ret->ssid);
			attemptedCachedSSID = false;
		}else{
			attemptedCachedSSID = true;
		}

		if(netSSID.empty()){
			state = Disconnected;
			Event evt{ .action = Event::Connect, .connect = { .success = false } };
			Events::post(Facility::WiFiSTA, evt);
			return;
		}

		state = Connecting;
		connectTries = 0;

		wifi_config_t cfg_sta = {
				.sta = {
						.password = "RoverRover"
				}
		};

		strcpy((char*) cfg_sta.sta.ssid, netSSID.c_str());
		ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &cfg_sta));
		esp_wifi_connect();
	}
}

esp_netif_t* WiFiSTA::createNetif(){
	esp_netif_inherent_config_t base = ESP_NETIF_INHERENT_DEFAULT_WIFI_STA();
	base.flags = (esp_netif_flags_t) ((base.flags & ~(ESP_NETIF_DHCP_SERVER | ESP_NETIF_DHCP_CLIENT | ESP_NETIF_FLAG_EVENT_IP_MODIFIED)) | ESP_NETIF_FLAG_GARP);

	const esp_netif_ip_info_t ip = {
			.ip =		{ .addr = esp_ip4addr_aton("11.0.0.2") },
			.netmask =	{ .addr = esp_ip4addr_aton("255.255.255.0") },
			.gw =		{ .addr = esp_ip4addr_aton("11.0.0.1") },
	};
	base.ip_info = &ip;

	esp_netif_obj* netif = esp_netif_create_wifi(WIFI_IF_STA, &base);
	esp_wifi_set_default_wifi_sta_handlers();

	return netif;
}

void WiFiSTA::connect(){
	if(state != Disconnected && state != ConnAbort) return;

	state = Scanning;
	const wifi_scan_config_t ScanConfig = {
			.channel = 1,
			.scan_type = WIFI_SCAN_TYPE_PASSIVE
	};
	esp_wifi_scan_start(&ScanConfig, false);
}

WiFiSTA::State WiFiSTA::getState(){
	return state;
}

ConnectionStrength WiFiSTA::getConnectionStrength(){
	hysteresis.update(-getConnectionRSSI());
	return (ConnectionStrength)hysteresis.get();
}

int WiFiSTA::getConnectionRSSI() const{
	//Needs ESP-IDF v5.1.1
/*	if(state != Connected){
		return -1;
	}

	int rssi = -1;
	esp_err_t error = esp_wifi_sta_get_rssi(&rssi);
	if(error != ESP_OK){
		return 0;
	}

	return rssi;*/
	return 0;
}

void WiFiSTA::disconnect(){
	if(state == Disconnected) return;

	cachedSSID = "";
	attemptedCachedSSID = false;

	switch(state){
		case Connected:
			esp_wifi_disconnect();
			break;
		case Connecting:
			state = ConnAbort;
			esp_wifi_disconnect();
			break;
		case Disconnected:
			break;
		case Scanning:
			state = ConnAbort;
			esp_wifi_scan_stop();
			break;
		case ConnAbort:
			break;
	}
}

void WiFiSTA::start() {
	ESP_ERROR_CHECK(esp_wifi_start());

	initSem.acquire();
}

void WiFiSTA::stop() {
	ESP_ERROR_CHECK(esp_wifi_stop());
}

wifi_ap_record_t* WiFiSTA::findNetwork(wifi_ap_record_t* ap_info, uint32_t numRecords){
	std::vector<wifi_ap_record_t*> networks;
	for(int i = 0; i < numRecords; ++i){
		if(strncmp((const char*) ap_info[i].ssid, "Perseverance Rover", 18) == 0){
			networks.emplace_back(&ap_info[i]);
		}
	}

	if (networks.empty()) {
		return nullptr;
	}

	std::remove(networks.begin(), networks.end(), nullptr);

	std::sort(networks.begin(), networks.end(), [](wifi_ap_record_t* first, wifi_ap_record_t* second) -> bool {
		return first->rssi > second->rssi;
	});

	return networks.front();
}