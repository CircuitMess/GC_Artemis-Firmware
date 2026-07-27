#include "../Client.h"
#include "Util/UUIDFmt.h"
#include <esp_log.h>

static const char* TAG = "BLE::Client::Service";

BLE::Client::Service::Service(esp_bt_uuid_t uuid) : uuid(uuid){

}

std::shared_ptr<BLE::Client::Char> BLE::Client::Service::addChar(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props){
	std::shared_ptr<Char> chr(new Char(uuid, props));
	chars.insert(chr);
	return chr;
}

void BLE::Client::Service::setOnConnectCb(Service::ConnectCB onConnectedCb){
	onConnectCB = std::move(onConnectedCb);
}

void BLE::Client::Service::setOnDisconnectCb(Service::DisconnectCB onDisconnectCb){
	onDisconnectCB = std::move(onDisconnectCb);
}

bool BLE::Client::Service::established(){
	return svc != nullptr;
}

bool BLE::Client::Service::populated(){
	for(const auto& chr : chars){
		if(chr->established()) return true;
	}

	return false;
}

void BLE::Client::Service::establish(std::unique_ptr<ServiceInfo> info){
	ESP_LOGI(TAG, "Established");
	svc = std::move(info);
}

void BLE::Client::Service::pull(){
	if(!svc) return;

	for(const auto& chr : chars){
		auto remote = svc->getCharByUUID(chr->uuid);
		if(remote.uuid.len == 0){
			ESP_LOGW(TAG, "Registered characteristic not found in remote service. Char UUID: %s",
				formatUUID128(chr->uuid.uuid.uuid128).c_str());
			continue;
		}

		// TODO: refactor this or something
		svc->regChar(chr.get(), remote.char_handle);
		chr->establish(svc->makeCharInfo(remote.char_handle), remote.properties);
	}

	if(onConnectCB){
		onConnectCB();
	}
}

void BLE::Client::Service::close(){
	if(!svc) return;
	ESP_LOGI(TAG, "Closing");

	for(const auto& chr : chars){
		chr->close();
	}

	svc.reset();

	if(onDisconnectCB){
		onDisconnectCB();
	}
}
