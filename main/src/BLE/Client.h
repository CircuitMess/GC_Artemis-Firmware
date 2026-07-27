#ifndef CLOCKSTAR_FIRMWARE_BLE_CLIENT_H
#define CLOCKSTAR_FIRMWARE_BLE_CLIENT_H

#include "Util/Queue.h"
#include "Util/PSRAMAllocator.h"
#include <cstdint>
#include <memory>
#include <vector>
#include <unordered_set>
#include <functional>
#include <esp_bt_defs.h>
#include <esp_gatt_defs.h>
#include <esp_gattc_api.h>

namespace BLE {

class GAP;

class Client {
public:
	class Char;
	class CharInfo;
	class Service;
	class ServiceInfo;
#include "Client/Char.h"
#include "Client/CharInfo.h"
#include "Client/Service.h"
#include "Client/ServiceInfo.h"

	Client(GAP* gap);
	~Client();

	std::shared_ptr<Service> addService(esp_bt_uuid_t uuid);

private:
	static Client* self;
	friend ServiceInfo;
	friend CharInfo;
	friend GAP;

	std::unordered_set<std::shared_ptr<Service>> services;
	std::unordered_map<uint16_t, Char*> chars;

	static constexpr int AppID = 0;
	struct InterfaceInfo {
		uint8_t appID = 0xff;
		uint8_t hndl = 0xff;

		operator bool(){ return appID != 0xff && hndl != 0; }
	} iface;

	struct ConnectionInfo {
		esp_bd_addr_t addr;
		uint16_t hndl = 0xffff;

		uint16_t MTU_size = 500;

		// Discovery is gated on BOTH the GATT connection being open and the link
		// being encrypted. These can complete in either order (a bonded iPhone may
		// auto-encrypt before ESP_GATTC_OPEN_EVT), so whichever finishes last
		// starts discovery; `discovering` guards against starting it twice.
		bool opened = false;
		bool paired = false;
		bool discovering = false;

		operator bool(){ return hndl != 0xffff; }
	} con;

	void ble_GATTC_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t *param);

	GAP* gap;
	void onPairDone();

	void onConnect(const esp_ble_gattc_cb_param_t::gattc_connect_evt_param* param);
	void onOpen(const esp_ble_gattc_cb_param_t::gattc_open_evt_param* param);
	void onMtuResp(const esp_ble_gattc_cb_param_t::gattc_cfg_mtu_evt_param* param);

	void maybeStartDiscovery();
	void searchServices();
	void onSearchResult(const esp_ble_gattc_cb_param_t::gattc_search_res_evt_param* param);
	void onSearchComplete(const esp_ble_gattc_cb_param_t::gattc_search_cmpl_evt_param* param);

	void onClose(const esp_ble_gattc_cb_param_t::gattc_close_evt_param* param);
	void onDisconnect(const esp_ble_gattc_cb_param_t::gattc_disconnect_evt_param* param);

	void passToChar(esp_gattc_cb_event_t event, esp_ble_gattc_cb_param_t *param);
	void close();

};

}


#endif //CLOCKSTAR_FIRMWARE_BLE_CLIENT_H
