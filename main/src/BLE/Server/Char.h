#ifdef CLOCKSTAR_FIRMWARE_BLE_SERVER_H


class Char {
public:

	using NotifRegCB = std::function<void(const esp_bd_addr_t)>;
	void setNotifRegCb(NotifRegCB cb);

	struct WriteMsg {
		PSRAMByteBuffer data;
		WriteMsg(PSRAMByteBuffer data) : data(std::move(data)){}
	};

	std::unique_ptr<WriteMsg> getNextWrite(TickType_t wait = portMAX_DELAY);

	void sendNotif(const uint8_t* data, size_t len);

	// Convenience overload: accepts any uint8_t vector (default- or PSRAM-allocated) and
	// forwards a view, so callers don't need to match the buffer's allocator type.
	template<typename Alloc>
	void sendNotif(const std::vector<uint8_t, Alloc>& data){
		sendNotif(data.data(), data.size());
	}

private:
	friend BLE::Server;
	friend BLE::Server::Service;
	Char(esp_bt_uuid_t uuid, esp_gatt_char_prop_t props);

	esp_bt_uuid_t uuid;
	esp_gatt_char_prop_t props;
	esp_gatt_perm_t perm = 0;

	PtrQueue<WriteMsg> writeQueue;

	std::unique_ptr<BLE::Server::CharInfo> chr;
	void establish(std::unique_ptr<BLE::Server::CharInfo> info);

	void onRead(const esp_ble_gatts_cb_param_t::gatts_read_evt_param* param);
	void onWrite(const esp_ble_gatts_cb_param_t::gatts_write_evt_param* param);
	void onExecWrite(const esp_ble_gatts_cb_param_t::gatts_exec_write_evt_param* param);
	void onDisconnect();

	bool notifyEn = false;
	bool indicateEn = false;

	NotifRegCB onNotifRegCB;

	uint16_t ctrlDescrHndl = 0xffff;

	static constexpr size_t MaxWriteData = 2 * 1024;
	PSRAMByteBuffer writeData;

};


#endif //CLOCKSTAR_FIRMWARE_BLE_SERVER_H
