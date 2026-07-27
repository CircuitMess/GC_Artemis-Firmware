#ifdef CLOCKSTAR_FIRMWARE_BLE_CLIENT_H

class CharInfo {
public:
	CharInfo(const BLE::Client* client, uint16_t hndl);

	void regForNotify();

	// Frees this characteristic's slot in bluedroid's notify registration table
	// (BTA_GATTC_NOTIF_REG_MAX entries). Purely a local table operation — safe to
	// call after the connection is gone. Must use the same address the slot was
	// registered with; bluedroid's own clear-on-disconnect can miss it when the
	// peer's RPA rotates, leaking the slot.
	void unregForNotify();

	void writeDescr(esp_bt_uuid_t uuid, uint8_t* data, size_t len);

	void write(uint8_t* data, size_t len, bool needResponse);

	void read();

private:
	const BLE::Client* client;

	const uint16_t hndl;

};

#endif //CLOCKSTAR_FIRMWARE_BLE_CLIENT_H
