#include <BLE/Server.h>
#include <esp_log.h>
#include <algorithm>
#include <cstring>

static const char* TAG = "BLE::Server::Service";

BLE::Server::Service::Service(esp_gatt_srvc_id_t id) : id(id){

}

std::shared_ptr<BLE::Server::Char> BLE::Server::Service::addChar(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props){
	std::shared_ptr<Char> chr(new Char(uuid, props));
	chars.push_back(chr);
	return chr;
}

void BLE::Server::Service::onDisconnect(){
	for(auto& chr : chars){
		chr->onDisconnect();
	}
}

void BLE::Server::Service::establish(uint16_t hndl){
	ESP_LOGI(TAG, "Established");
	this->hndl = hndl;

	esp_ble_gatts_start_service(hndl);

	// Bluedroid attaches a descriptor to the most recently added char in the service
	// ("follow" rule). Char::establish auto-adds a CCCD for chars with NOTIFY/INDICATE,
	// so those chars must be added last for their CCCD to land on them in the GATT
	// table. See the pendingDescrs comment in Server.h for the full rationale.
	std::stable_sort(chars.begin(), chars.end(), [](const std::shared_ptr<Char>& a, const std::shared_ptr<Char>& b){
		constexpr esp_gatt_char_prop_t descrBearing = ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_INDICATE;
		return !(a->props & descrBearing) && (b->props & descrBearing);
	});

	for(const auto& chr : chars){
		esp_ble_gatts_add_char(hndl, &chr->uuid, chr->perm, chr->props, nullptr, nullptr);
	}
}

std::shared_ptr<BLE::Server::Char> BLE::Server::Service::charCreated(esp_gatt_status_t status, esp_bt_uuid_t uid, std::unique_ptr<BLE::Server::CharInfo> charInfo){
	auto it = std::find_if(chars.cbegin(), chars.cend(), [uid](const std::shared_ptr<Char>& chr){
		const auto len = std::min(uid.len, chr->uuid.len);
		return memcmp(uid.uuid.uuid128, chr->uuid.uuid.uuid128, len) == 0;
	});
	if(it == chars.end()){
		ESP_LOGW(TAG, "got handle for non-existant service");
		return nullptr;
	}
	auto chr = *it;

	chr->establish(std::move(charInfo));
	return chr;
}
