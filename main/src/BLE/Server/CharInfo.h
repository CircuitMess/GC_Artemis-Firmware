#ifdef CLOCKSTAR_FIRMWARE_BLE_SERVER_H

class CharInfo {
public:
	CharInfo(BLE::Server* server, const BLE::Server::Service* service, uint16_t hndl);

	void addDescr(esp_bt_uuid_t uuid, esp_gatt_perm_t perm);

	// `conn_id` is the connection that originated the request being answered.
	esp_err_t sendResp(uint16_t conn_id, uint32_t trans, esp_gatt_status_t status, esp_gatt_rsp_t* resp = nullptr);

	// Sends a notification on this characteristic to the connected peer; if there is no
	// connection, drops silently. Reads directly from the caller's buffer (no copy) and
	// fragments it into MTU-sized chunks.
	void sendNotif(const uint8_t* data, size_t len);

private:
	BLE::Server* server;
	const BLE::Server::Service* service;

	const uint16_t hndl;

};

#endif //CLOCKSTAR_FIRMWARE_BLE_CLIENT_H
