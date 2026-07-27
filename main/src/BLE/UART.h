#ifndef CLOCKSTAR_FIRMWARE_UART_H
#define CLOCKSTAR_FIRMWARE_UART_H

#include "Server.h"
#include "Util/Threaded.h"
#include "Util/PSRAMAllocator.h"
#include <vector>

namespace BLE {

class UART : private Threaded {
public:
	UART(Server* server);
	virtual ~UART();

	void printf(const char* fmt, ...);
	void print(const std::vector<uint8_t>& data);

	// Returns contents of RX buffer and clears it
	std::vector<uint8_t> scan();

	/**
	 * Waits until a new-line character, and returns the contents including the nl char.
	 * May contain multiple lines. This is a blocking function.
	 */
	std::unique_ptr<PSRAMByteBuffer> scan_nl(TickType_t wait = portMAX_DELAY);

private:
	BLE::Server* server;

	std::shared_ptr<BLE::Server::Service> service;
	std::shared_ptr<BLE::Server::Char> txChar;
	std::shared_ptr<BLE::Server::Char> rxChar;


	PSRAMByteBuffer txBuf;
	PSRAMByteBuffer rxBuf;
	using Line = PSRAMByteBuffer;
	PtrQueue<Line> rxQueue;

	void loop() override;

	// Nordic UART
	static constexpr esp_bt_uuid_t ServiceUID = {
			.len = ESP_UUID_LEN_128,
			.uuid = { .uuid128 = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x01, 0x00, 0x40, 0x6E }}
	};

	// Server-side RX and TX
	static constexpr esp_bt_uuid_t RxCharUID = {
			.len = ESP_UUID_LEN_128,
			.uuid = { .uuid128 = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x02, 0x00, 0x40, 0x6E }}
	};static constexpr esp_bt_uuid_t TxCharUID = {
			.len = ESP_UUID_LEN_128,
			.uuid = { .uuid128 = { 0x9E, 0xCA, 0xDC, 0x24, 0x0E, 0xE5, 0xA9, 0xE0, 0x93, 0xF3, 0xA3, 0xB5, 0x03, 0x00, 0x40, 0x6E }}
	};

};

}


#endif //CLOCKSTAR_FIRMWARE_UART_H
