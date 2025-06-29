#include "Client.h"
#include "GAP.h"
#include <cstring>
#include <esp_log.h>
#include <esp_gap_ble_api.h>

static const char* TAG = "BLE::Client";

BLE::Client* BLE::Client::self = nullptr;

BLE::Client::Client(GAP* gap) : gap(gap){
	if(self != nullptr){
		ESP_LOGE(TAG, "Client already exists");
		return;
	}
	self = this;

	esp_ble_gattc_register_callback([](esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param){
		if(self == nullptr) return;
		self->ble_GATTC_cb(event, gattc_if, param);
	});

	if(iface.appID == 0xff){
		iface.appID = AppID;
		esp_ble_gattc_app_register(AppID);
	}

	// TODO: This is only needed so GAP can notify the GATT Client when pairing is done
	gap->setClient(this);
}

BLE::Client::~Client(){
	self = nullptr;
	gap->setClient(nullptr);
}

std::shared_ptr<BLE::Client::Service> BLE::Client::addService(esp_bt_uuid_t uuid){
	std::shared_ptr<Service> srv(new Service(uuid));
	services.insert(srv);
	return srv;
}

void BLE::Client::ble_GATTC_cb(esp_gattc_cb_event_t event, esp_gatt_if_t gattc_if, esp_ble_gattc_cb_param_t* param){
	/* 0xff, not specify a certain gatt_if, need to call every profile cb function */
	if(gattc_if == 0xff){
		printf("GATTC CB interface 0xff - event %d\n", event);
	}

	if(event == ESP_GATTC_REG_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_REG_EVT");

		if(param->reg.app_id != iface.appID){
			ESP_LOGE(TAG, "CB: App ID missmatch. Received %d, have %d. Status is %d", param->reg.app_id, iface.appID, param->reg.status);
			return;
		}

		if(param->reg.status == ESP_GATT_OK){
			ESP_LOGI(TAG, "App registered");
			iface.hndl = gattc_if;
		}else{
			ESP_LOGW(TAG, "Reg app failed, app_id %04x, status %d", param->reg.app_id, param->reg.status);
			iface = {};
			return;
		}
	}else if(event == ESP_GATTC_CONNECT_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_CONNECT_EVT");
		onConnect(&param->connect);
	}else if(event == ESP_GATTC_OPEN_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_OPEN_EVT");
		onOpen(&param->open);
	}else if(event == ESP_GATTC_CFG_MTU_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_CFG_MTU_EVT");
		onMtuResp(&param->cfg_mtu);
	}else if(event == ESP_GATTC_SEARCH_RES_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_SEARCH_RES_EVT");
		onSearchResult(&param->search_res);
	}else if(event == ESP_GATTC_SEARCH_CMPL_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_SEARCH_CMPL_EVT");
		onSearchComplete(&param->search_cmpl);
	}else if(event == ESP_GATTC_CLOSE_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_CLOSE_EVT");
		onClose(&param->close);
	}else if(event == ESP_GATTC_DISCONNECT_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_DISCONNECT_EVT");
		onDisconnect(&param->disconnect);
	}else{
		switch(event){
			case ESP_GATTC_REG_FOR_NOTIFY_EVT:
			case ESP_GATTC_NOTIFY_EVT:
			case ESP_GATTC_WRITE_CHAR_EVT:
			case ESP_GATTC_READ_CHAR_EVT:
				passToChar(event, param);
				break;
			default:
				ESP_LOGV(TAG, "Unhandled event: %d", event);
				break;
		}
	}
}

void BLE::Client::onConnect(const esp_ble_gattc_cb_param_t::gattc_connect_evt_param* param){
	memcpy(con.addr, param->remote_bda, 6);

	// Server notifies ConMan, which in turn sets connection parameters

	esp_ble_set_encryption(con.addr, ESP_BLE_SEC_ENCRYPT_MITM);
	// ble_set_encryption initiates pairing. After the devices complete pairing,
	// the ESP_GAP_BLE_AUTH_CMPL_EVT comes up (handled by BLE), and BLE calls onPairDone().
}

void BLE::Client::onPairDone(){
	esp_ble_gattc_open(iface.hndl, con.addr, BLE_ADDR_TYPE_PUBLIC, true);
}

void BLE::Client::onOpen(const esp_ble_gattc_cb_param_t::gattc_open_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "open failed, error status = 0x%x", param->status);
		return;
	}

	con.hndl = param->conn_id;

	esp_ble_gattc_send_mtu_req(iface.hndl, con.hndl);
}

void BLE::Client::onMtuResp(const esp_ble_gattc_cb_param_t::gattc_cfg_mtu_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "config mtu failed, error status = 0c%x", param->status);
		return;
	}

	con.MTU_size = param->mtu; // TODO: check if this really sets the MTU on the remote device side

	searchServices();
}

void BLE::Client::searchServices(){
	for(const auto& service : services){
		esp_ble_gattc_search_service(iface.hndl, con.hndl, &service->uuid);
	}
}

void BLE::Client::onSearchResult(const esp_ble_gattc_cb_param_t::gattc_search_res_evt_param* param){
	for(const auto& service : services){
		if(param->srvc_id.uuid.len == service->uuid.len && memcmp(param->srvc_id.uuid.uuid.uuid128, service->uuid.uuid.uuid128, service->uuid.len) == 0){
			service->establish(std::make_unique<ServiceInfo>(this, param->start_handle, param->end_handle));
		}
	}
}

void BLE::Client::onSearchComplete(const esp_ble_gattc_cb_param_t::gattc_search_cmpl_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "search service failed, error status = %x", param->status);
		return;
	}

	// TODO: invoke pull on the service which search results belong to
	// current implementation only works with one service registered, I think
	for(auto& svc : services){
		if(!svc->established()) continue;
		if(svc->populated()) continue;
		svc->pull();
	}

	// TODO: disconnect if no registered service is found on remote server
}

void BLE::Client::onClose(const esp_ble_gattc_cb_param_t::gattc_close_evt_param* param){
	if(param->status != ESP_GATT_OK){
		ESP_LOGE(TAG, "close failed, error status = %x", param->status);
		return;
	}

	close();
}

void BLE::Client::onDisconnect(const esp_ble_gattc_cb_param_t::gattc_disconnect_evt_param* param){
	ESP_LOGI(TAG, "Disconnected. Reason: 0x%x", param->reason);
	close();
}

void BLE::Client::passToChar(esp_gattc_cb_event_t event, esp_ble_gattc_cb_param_t* param){
#define check(x) do { if(x == chars.end()){ ESP_LOGW(TAG, "Received event %d directed to non-registered characteristic", event); return; } } while(0)

	if(event == ESP_GATTC_REG_FOR_NOTIFY_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_REG_FOR_NOTIFY_EVT");

		auto chr = chars.find(param->reg_for_notify.handle);
		check(chr);

		chr->second->onRegNotify(&param->reg_for_notify);
	}else if(event == ESP_GATTC_NOTIFY_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_NOTIFY_EVT");

		auto chr = chars.find(param->notify.handle);
		check(chr);

		chr->second->onNotify(&param->notify);
	}else if(event == ESP_GATTC_READ_CHAR_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_READ_CHAR_EVT");

		auto chr = chars.find(param->read.handle);
		check(chr);

		// TODO: lol what is this, turn this into actual read evt handling
		esp_ble_gattc_cb_param_t::gattc_notify_evt_param notifParam = {
				.value_len = param->read.value_len,
				.value = param->read.value,
				.is_notify = true
		};
		chr->second->onNotify(&notifParam);
	}else if(event == ESP_GATTC_WRITE_CHAR_EVT){
		ESP_LOGI(TAG, "ESP_GATTC_WRITE_CHAR_EVT");

		auto chr = chars.find(param->write.handle);
		check(chr);

		chr->second->onWriteResp(event, &param->write);

	}else{
		ESP_LOGW(TAG, "Unhandled characteristic event: 0x%x", event);
	}
}

void BLE::Client::close(){
	for(auto& svc : services){
		if(svc == nullptr) {
			continue;
		}

		svc->close();
	}
	chars.clear();

	con.hndl = 0;
	memset(con.addr, 0, 6);

	// Server notifies ConMan, which in turn starts advertising
}
