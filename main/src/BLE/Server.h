#ifndef CLOCKSTAR_FIRMWARE_BLE_SERVER_H
#define CLOCKSTAR_FIRMWARE_BLE_SERVER_H

#include "Util/Queue.h"
#include "Util/PSRAMAllocator.h"
#include <cstdint>
#include <cstring>
#include <mutex>
#include <vector>
#include <memory>
#include <functional>
#include <queue>
#include <unordered_set>
#include <unordered_map>
#include <esp_bt_defs.h>
#include <esp_gatt_defs.h>
#include <esp_gatts_api.h>

namespace BLE {

class GAP;

class Server {
public:
	class Service;
	class Char;
	class CharInfo;
#include "Server/Service.h"
#include "Server/Char.h"
#include "Server/CharInfo.h"

	Server(GAP* gap);
	virtual ~Server();

	std::shared_ptr<Service> addService(esp_bt_uuid_t uuid);

	void start();

	using ConnectCB = std::function<void(const esp_bd_addr_t)>;
	using DisconnectCB = std::function<void(const esp_bd_addr_t)>;
	using SubHandle = uint32_t;
	SubHandle addOnConnectCb(ConnectCB cb);
	SubHandle addOnDisconnectCb(DisconnectCB cb);
	void removeOnConnectCb(SubHandle handle);
	void removeOnDisconnectCb(SubHandle handle);

private:
	static Server* self;
	friend CharInfo;

	std::unordered_set<std::shared_ptr<Service>> services;
	std::unordered_map<uint16_t, Char*> chars;

	// Serialized queue of pending descriptor add requests.
	//
	// Bluedroid associates a descriptor with the most recently added characteristic in
	// the same service ("follow" semantics), and ESP_GATTS_ADD_CHAR_DESCR_EVT does not
	// carry the parent characteristic handle (espressif/esp-idf#1484). To map each event
	// back to the characteristic that asked for it, we keep at most one outstanding
	// esp_ble_gatts_add_char_descr at a time: the head of the queue is the request being
	// answered by the next ESP_GATTS_ADD_CHAR_DESCR_EVT.
	//
	// All BLE API calls and bluedroid callbacks share the BTC task thread, so the queue
	// needs no locking. The application is also expected to drive the BLE abstraction
	// from a single thread, regardless of whether Client and Server are both in use.
	//
	// CAVEAT: this only works if every service has at most one descriptor-bearing
	// characteristic (i.e. one with the NOTIFY or INDICATE property bit set).
	//
	// TODO: replace this with esp_ble_gatts_create_attr_tab
	// The attribute-table API builds the whole service+chars+descriptors atomically
	// and reports all handles in a single event, side-stepping the "follow" rule and
	// this serialization entirely.
	struct DescrReq {
		uint16_t charHndl;
		uint16_t serviceHndl;
		esp_bt_uuid_t uuid;
		esp_gatt_perm_t perm;
	};
	std::queue<DescrReq> pendingDescrs;
	bool descrInFlight = false;

	void queueDescr(DescrReq req);
	void tryIssueNextDescr();

	// Callback maps are mutated from app threads (construction & destruction of Android
	// et al.) and iterated on the BTC task in onConnect / onDisconnect. Guard with a
	// mutex; iterate over a snapshot taken under the lock to avoid holding it across
	// user callbacks.
	std::unordered_map<SubHandle, ConnectCB> onConnectCBs;
	std::unordered_map<SubHandle, DisconnectCB> onDisconnectCBs;
	SubHandle nextSubHandle = 1;
	mutable std::mutex cbMut;

	friend GAP;
	GAP* gap;
	void onPairDone();

	static constexpr int AppID = 1;
	struct InterfaceInfo {
		uint8_t appID = 0xff;
		uint8_t hndl = 0xff;

		operator bool(){ return appID != 0xff && hndl != 0; }
	} iface;

	struct ConnectionInfo {
		esp_bd_addr_t addr;
		uint16_t hndl = 0xffff;

		uint16_t MTU_size = 23;

		operator bool(){ return hndl != 0xffff; }
	} con;

	void ble_GATTS_cb(esp_gatts_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gatts_cb_param_t *param);

	void registerServices();
	void onServiceCreated(const esp_ble_gatts_cb_param_t::gatts_create_evt_param* param);
	void onCharCreated(const esp_ble_gatts_cb_param_t::gatts_add_char_evt_param* param);
	void onCharDescrCreated(const esp_ble_gatts_cb_param_t::gatts_add_char_descr_evt_param* param);

	void onMtuResp(const esp_ble_gatts_cb_param_t::gatts_mtu_evt_param* param);
	void onConnect(const esp_ble_gatts_cb_param_t::gatts_connect_evt_param* param);
	void onDisconnect(const esp_ble_gatts_cb_param_t::gatts_disconnect_evt_param* param);

	void passToChar(esp_gatts_cb_event_t event, const esp_ble_gatts_cb_param_t* param);

};

}

#endif //CLOCKSTAR_FIRMWARE_BLE_SERVER_H
