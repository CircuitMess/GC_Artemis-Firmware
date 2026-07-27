#include "UART.h"

BLE::UART::UART(BLE::Server* server) : Threaded("BLE UART", 2 * 1024), server(server), rxQueue(12){
	service = server->addService(ServiceUID);

	// Creation order is important due to char shenanigans: When any char creates a descriptor,
	// that descriptor gets assigned to the first char created for the service. Chars with NOTIFY
	// bit create descriptors that are used by clients to sub for notifs, so the TX char with
	// NOTIFY bit should be created first.
	txChar = service->addChar(TxCharUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY);
	rxChar = service->addChar(RxCharUID, ESP_GATT_CHAR_PROP_BIT_WRITE);

	rxBuf.reserve(1024);
	txBuf.reserve(256);

	start();
}

BLE::UART::~UART(){
	stop();
}

void BLE::UART::printf(const char* fmt, ...){
	txBuf.resize(txBuf.capacity());

	va_list argptr;
	va_start(argptr, fmt);
	int len = vsnprintf((char*) txBuf.data(), txBuf.size(), fmt, argptr);
	va_end(argptr);

	txBuf.resize(len);
	txChar->sendNotif(txBuf);
}

void BLE::UART::print(const std::vector<uint8_t>& data){
	txChar->sendNotif(data);
}

std::vector<uint8_t> BLE::UART::scan(){
	if(rxBuf.empty()) return {};
	std::vector<uint8_t> ret(rxBuf.begin(), rxBuf.end());
	rxBuf.clear();
	return ret;
}

void BLE::UART::loop(){
	auto msg = rxChar->getNextWrite();
	if(msg == nullptr) return;

	rxBuf.insert(rxBuf.end(), msg->data.cbegin(), msg->data.cend());

	auto nl = std::find(rxBuf.begin(), rxBuf.end(), '\n');
	if(nl != rxBuf.end()){
		do {
			// TODO: Possible fragmentation: lines are created on heap
			auto line = std::make_unique<Line>(rxBuf.begin(), nl+1);
			rxBuf.erase(rxBuf.begin(), nl+1);

			rxQueue.post(std::move(line));

			nl = std::find(rxBuf.begin(), rxBuf.end(), '\n');
		} while(nl != rxBuf.end());
	}
}

std::unique_ptr<BLE::UART::Line> BLE::UART::scan_nl(TickType_t wait){
	auto line = rxQueue.get(wait);
	if(!line) return nullptr;

	return line;
}
