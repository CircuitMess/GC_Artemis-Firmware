#ifdef CLOCKSTAR_FIRMWARE_BLE_SERVER_H


class Service {
public:
	std::shared_ptr<BLE::Server::Char> addChar(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props = 0);

private:
	friend BLE::Server;

	Service(esp_gatt_srvc_id_t id);

	esp_gatt_srvc_id_t id;
	std::vector<std::shared_ptr<BLE::Server::Char>> chars;

	uint16_t hndl = 0xffff;
	void establish(uint16_t hndl);
	std::shared_ptr<Server::Char> charCreated(esp_gatt_status_t status, esp_bt_uuid_t uid, std::unique_ptr<BLE::Server::CharInfo> charInfo);

	void onDisconnect();

};


#endif //CLOCKSTAR_FIRMWARE_BLE_SERVER_H
