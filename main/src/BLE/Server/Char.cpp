#include "BLE/Server.h"
#include <esp_log.h>
#include <cstring>

static const char* TAG = "BLE::Server::Char";

BLE::Server::Char::Char(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props) : uuid(uuid), props(props), writeQueue((props & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR)) ? 12 : 1){
	if(props & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR)){
		// Only write-capable chars ever accumulate prepared writes; reserve (in PSRAM) only
		// for them so notify/read-only chars don't hold a 2KB buffer they can never use.
		writeData.reserve(MaxWriteData);
		perm |= ESP_GATT_PERM_WRITE;
	}
	if(props & (ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE)){
		perm |= ESP_GATT_PERM_READ;
	}
}

void BLE::Server::Char::setNotifRegCb(NotifRegCB cb){
	if(!(props & (ESP_GATT_CHAR_PROP_BIT_NOTIFY))){
		ESP_LOGW(TAG, "Set NotifReg CB, but NOTIFY property bit isn't set");
		return;
	}

	onNotifRegCB = std::move(cb);
}

std::unique_ptr<BLE::Server::Char::WriteMsg> BLE::Server::Char::getNextWrite(TickType_t wait){
	if(!(props & (ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_WRITE_NR))){
		ESP_LOGW(TAG, "Requesting write msg, but WRITE property bit isn't");
		return nullptr;
	}

	return writeQueue.get(wait);
}

void BLE::Server::Char::sendNotif(const uint8_t* data, size_t len){
	if(!(props & (ESP_GATT_CHAR_PROP_BIT_NOTIFY))){
		ESP_LOGW(TAG, "Sending notif, but NOTIFY property bit isn't set");
		return;
	}

	if(!notifyEn) return;

	chr->sendNotif(data, len);
}

void BLE::Server::Char::establish(std::unique_ptr<BLE::Server::CharInfo> info){
	ESP_LOGI(TAG, "Established");
	this->chr = std::move(info);

	if(props & (ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE)){
		esp_bt_uuid_t uuid = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = ESP_GATT_UUID_CHAR_CLIENT_CONFIG } };
		chr->addDescr(uuid, ESP_GATT_PERM_WRITE | ESP_GATT_PERM_READ);
	}
}

void BLE::Server::Char::onRead(const esp_ble_gatts_cb_param_t::gatts_read_evt_param* param){

}

void BLE::Server::Char::onWrite(const esp_ble_gatts_cb_param_t::gatts_write_evt_param* param){
	// CCCD writes (NOTIFY/INDICATE enable/disable) target the descriptor's own handle.
	// Handle the descriptor branch fully and return — falling through would post the
	// 2-byte CCCD value into writeQueue and surface it to the app as a data write.
	if(!param->is_prep && param->len == 2 && param->handle == ctrlDescrHndl){
		const uint16_t val = param->value[1] << 8 | param->value[0];
		if(val == 0x0001){
			if(props & ESP_GATT_CHAR_PROP_BIT_NOTIFY){
				ESP_LOGI(TAG, "Client enabling NOTIFY");
				notifyEn = true;

				if(onNotifRegCB){
					onNotifRegCB(param->bda);
				}
			}else{
				ESP_LOGW(TAG, "Client enabling NOTIFY, but prop bit isn't set");
			}
		}else if(val == 0x0002){
			if(props & ESP_GATT_CHAR_PROP_BIT_INDICATE){
				ESP_LOGI(TAG, "Client enabling INDICATE");
				indicateEn = true;
			}else{
				ESP_LOGW(TAG, "Client enabling INDICATE, but prop bit isn't set");
			}
		}else if(val == 0x0000){
			ESP_LOGI(TAG, "Client disabling NOTIFY/INDICATE");
			notifyEn = false;
			indicateEn = false;
		}else{
			ESP_LOGW(TAG, "Unknown CCCD value 0x%04x", val);
		}

		if(param->need_rsp){
			chr->sendResp(param->conn_id, param->trans_id, ESP_GATT_OK);
		}
		return;
	}

	auto resp = [this, param](esp_gatt_status_t status, esp_gatt_rsp_t* resp = nullptr){
		if(!param->need_rsp) return true;
		return chr->sendResp(param->conn_id, param->trans_id, status, resp) == ESP_OK;
	};

	if(param->is_prep){
		if(param->offset > writeData.size()){
			resp(ESP_GATT_INVALID_OFFSET);
			return;
		}

		if(param->offset + param->len > MaxWriteData){
			resp(ESP_GATT_INVALID_ATTR_LEN);
			return;
		}

		esp_gatt_rsp_t rsp = {
				.attr_value = {
						.handle = param->handle,
						.offset = param->offset,
						.len = param->len,
						.auth_req = ESP_GATT_AUTH_REQ_MITM
				}
		};
		memcpy(rsp.attr_value.value, param->value, param->len);

		if(!resp(ESP_GATT_OK, &rsp)) return;

		writeData.insert(writeData.end(), param->value, param->value + param->len);
	}else{
		resp(ESP_GATT_OK);
		writeQueue.post(std::make_unique<WriteMsg>(PSRAMByteBuffer(param->value, param->value + param->len)), 0);
	}
}

void BLE::Server::Char::onExecWrite(const esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param* param){
	if(writeData.empty()) return;

	chr->sendResp(param->conn_id, param->trans_id, ESP_GATT_OK);
	writeQueue.post(std::make_unique<WriteMsg>(PSRAMByteBuffer(writeData.cbegin(), writeData.cend())), 0);
	writeData.clear();
}

void BLE::Server::Char::onDisconnect(){
	// CCCD state is per-connection; the peer must re-enable on reconnect, so drop our bookkeeping.
	notifyEn = false;
	indicateEn = false;
	writeData.clear();
}
