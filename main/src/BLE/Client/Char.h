#ifdef CLOCKSTAR_FIRMWARE_BLE_CLIENT_H

class Char {
public:

	struct Notif {
		PSRAMByteBuffer data;
		bool isIndicate;
		Notif(PSRAMByteBuffer data, bool isIndicate) : data(std::move(data)), isIndicate(isIndicate){}
	};

	/** Whether Characteristic is associated with the remote */
	bool established();

	/** Whether Characteristic is associated with the remote, and all configs are done */
	bool connected();

	using ConnectedCB = std::function<void()>;

	/** Gets next notification in the queue. This is a blocking function. */
	std::unique_ptr<Notif> getNextNotif(TickType_t wait = portMAX_DELAY);

	void setOnConnectedCb(ConnectedCB cb);

	void writeDescr(uint16_t uuid, const std::vector<uint8_t>& data);

	/** Returns true if the write was actually issued to the stack (connected and WRITE-capable). */
	bool write(const std::vector<uint8_t>& data);

	void read();

private:
	friend BLE::Client;
	friend BLE::Client::Service;
	Char(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props);

	esp_bt_uuid_t uuid;
	esp_gatt_char_prop_t props;
	esp_gatt_char_prop_t remoteProps;

	std::unique_ptr<BLE::Client::CharInfo> chr;

	void establish(std::unique_ptr<BLE::Client::CharInfo> info, esp_gatt_char_prop_t rProps);
	void close();

	enum class Config {
		Notify,
		COUNT
	};
	std::unordered_set<Config> configsDone;
	void configDone(Config config);

	PtrQueue<Notif> notifQueue;
	ConnectedCB onConnectedCB;

	void onNotify(const esp_ble_gattc_cb_param_t::gattc_notify_evt_param* param);
	void onRegNotify(const esp_ble_gattc_cb_param_t::gattc_reg_for_notify_evt_param* param);
	void onWriteResp(esp_gattc_cb_event_t evt, const esp_ble_gattc_cb_param_t::gattc_write_evt_param* param);

};


#endif //CLOCKSTAR_FIRMWARE_BLE_CLIENT_H
