#ifndef CLOCKSTAR_FIRMWARE_GAP_H
#define CLOCKSTAR_FIRMWARE_GAP_H

#define BLE_42_FEATURE_SUPPORT TRUE
#include <esp_gap_ble_api.h>
#include <unordered_set>
#include <esp_gattc_api.h>

namespace BLE {

class Client;
class Server;

class GAP {
public:
	GAP();
	virtual ~GAP();

private:
	static GAP* self;

	void ble_GAP_cb(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t* param);

	void initSecure();

	void startAdvertising();

	enum class Config {
		Advertising,
		ScanResponse,
		COUNT
	};
	std::unordered_set<Config> configsDone;
	void configDone(Config config);

	friend Client;
	Client* client = nullptr;
	void setClient(Client* client);

	friend Server;
	Server* server = nullptr;
	void setServer(Server* client);

	/** ### BLE device and advertising parameters ### */

	static constexpr uint8_t ServiceUUID[] = {
			/* LSB <--------------------------------------------------------------------------------> MSB */
			// first uuid, 16bit, [12],[13] is the value
			0xfb, 0x34, 0x9b, 0x5f, 0x80, 0x00, 0x00, 0x80, 0x00, 0x10, 0x00, 0x00, 0x12, 0x18, 0x00, 0x00
			// Nordic UART:
			// 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E
	};

	static constexpr esp_ble_adv_params_t AdvertParams = {
			.adv_int_min        = 0x20, // 20ms = 32 (0x20) * 0.625ms
			.adv_int_max        = 0x20, // Apple: After 30 seconds of no connection, feel free to switch to 1285ms (0x808)
			.adv_type           = ADV_TYPE_IND,
			.own_addr_type      = BLE_ADDR_TYPE_PUBLIC,
			.channel_map        = ADV_CHNL_ALL,
			.adv_filter_policy  = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
	};

	static constexpr esp_ble_adv_data_t AdvertConfig = {
			.set_scan_rsp = false,
			.include_txpower = false,
			.min_interval = 0x0006, // slave connection min interval, Time = min_interval * 1.25 msec
			.max_interval = 0x0010, // slave connection max interval, Time = max_interval * 1.25 msec
			.appearance = ESP_BLE_APPEARANCE_GENERIC_WATCH,
			.service_uuid_len = sizeof(ServiceUUID),
			.p_service_uuid = (uint8_t*) ServiceUUID,
			.flag = (ESP_BLE_ADV_FLAG_GEN_DISC | ESP_BLE_ADV_FLAG_BREDR_NOT_SPT)
	};

	static constexpr esp_ble_adv_data_t AdvertRespConfig = {
			.set_scan_rsp = true,
			.include_name = true
	};

};

}


#endif //CLOCKSTAR_FIRMWARE_GAP_H
