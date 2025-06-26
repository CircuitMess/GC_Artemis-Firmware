#include "CurrentTime.h"
#include "Services/Time.h"
#include "Util/Services.h"

CurrentTime::CurrentTime(BLE::Client* client) : Threaded("CurrentTime", 3 * 1024){
	service = client->addService(ServiceUUID);
	chr = service->addChar(CharUUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY | ESP_GATT_CHAR_PROP_BIT_READ);

	chr->setOnConnectedCb([this](){ chr->writeDescr(ESP_GATT_UUID_CHAR_CLIENT_CONFIG, { 0x01, 0x00 }); chr->read(); });

	start();
}

void CurrentTime::loop(){
	if(chr == nullptr || !chr->connected()){
		vTaskDelay(500);
		return;
	}

	auto notif = chr->getNextNotif();
	if(!notif){
		vTaskDelay(500);
		return;
	}

	setTime(notif->data);
}

void CurrentTime::setTime(const std::vector<uint8_t>& data){
	if(data.size() < 7) return;

	int year = data[0] | (data[1] << 8);
	int month = data[2];
	int day = data[3];
	int hour = data[4];
	int minute = data[5];
	int second = data[6];
	//int weekday = data[7];
	//int frac256 = data[8];

	tm time = {
			.tm_sec = second,
			.tm_min = minute,
			.tm_hour = hour,
			.tm_mday = day,
			.tm_mon = month-1,
			.tm_year = year - 1900
	};

	auto ts = static_cast<Time*>(Services.get(Service::Time));
	ts->setTime((tm) time);
}
