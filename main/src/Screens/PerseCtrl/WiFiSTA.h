#ifndef PERSE_ROVER_WIFIAP_H
#define PERSE_ROVER_WIFIAP_H

#include <esp_event.h>
#include <esp_netif_types.h>
#include <semaphore>
#include <esp_wifi_types.h>
#include <CommData.h>
#include "Util/Threaded.h"
#include "Util/Hysteresis.h"
#include <string>
#include <semaphore>

class WiFiSTA {
public:
	WiFiSTA();
	~WiFiSTA();

	struct Event {
		enum { Connect, Disconnect, Probe } action;
		union {
			struct {
				uint8_t bssid[6];
				bool success;
			} connect;

			struct {
				uint8_t bssid[6];
			} disconnect;
		};
	};

	bool hasCachedSSID() const;
	void connect();
	void disconnect();

	void start();
	void stop();

	enum State { Connected, Connecting, Disconnected, Scanning, ConnAbort };
	State getState();

	ConnectionStrength getConnectionStrength();

private:
	int getConnectionRSSI() const;

	esp_event_handler_instance_t evtHandler;
	void event(int32_t id, void* data);

	esp_netif_t* netif = nullptr;

	State state = Disconnected;
	std::binary_semaphore initSem{ 0 };

	std::string cachedSSID;
	bool attemptedCachedSSID = false;

	Hysteresis hysteresis;

	static constexpr int ConnectRetries = 2;
	int connectTries;

	static esp_netif_t* createNetif();

	static constexpr uint16_t ScanListSize = 24;

	inline static bool eventLoopCreated = false;

	static wifi_ap_record_t* findNetwork(wifi_ap_record_t* ap_info, uint32_t numRecords);
};


#endif //PERSE_ROVER_WIFIAP_H
