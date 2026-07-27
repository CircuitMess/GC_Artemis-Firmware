#ifndef CLOCKSTAR_FIRMWARE_CURRENTTIME_H
#define CLOCKSTAR_FIRMWARE_CURRENTTIME_H

#include "BLE/Client.h"
#include "Util/Threaded.h"
#include "Util/PSRAMAllocator.h"

class CurrentTime : public Threaded {
public:
	CurrentTime(BLE::Client* client);

private:
	std::shared_ptr<BLE::Client::Service> service;
	std::shared_ptr<BLE::Client::Char> chr;

	void loop() override;

	void setTime(const PSRAMByteBuffer& data);

	static constexpr esp_bt_uuid_t ServiceUUID = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = 0x1805 }};
	static constexpr esp_bt_uuid_t CharUUID = { .len = ESP_UUID_LEN_16, .uuid = { .uuid16 = 0x2A2B }};

};


#endif //CLOCKSTAR_FIRMWARE_CURRENTTIME_H
