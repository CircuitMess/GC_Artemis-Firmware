#include "Client.h"
#include <cstring>
#include <esp_log.h>

static const char* TAG = "ANCS";

ANCS::Client::Client(BLE::Client* client) : notifThread([this](){ loopNotif(); }, "ANCS::Notif", 2 * 1024), dataThread([this](){ loopData(); }, "ANCS::Data", 3 * 1024){
	service = client->addService(ServiceUUID);
	chr.notif = service->addChar(Char_NotifSource_UUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY);
	chr.ctrl = service->addChar(Char_ControlPoint_UUID, ESP_GATT_CHAR_PROP_BIT_WRITE);
	chr.data = service->addChar(Char_DataSource_UUID, ESP_GATT_CHAR_PROP_BIT_NOTIFY);

	service->setOnConnectCb([this](){ onConn(); });
	service->setOnDisconnectCb([this](){ onDiscon(); });

	chr.notif->setOnConnectedCb([this](){ chr.notif->writeDescr(ESP_GATT_UUID_CHAR_CLIENT_CONFIG, { 0x01, 0x00 }); });
	chr.data->setOnConnectedCb([this](){ chr.data->writeDescr(ESP_GATT_UUID_CHAR_CLIENT_CONFIG, { 0x01, 0x00 }); });

	notifThread.start();
	dataThread.start();
}

ANCS::Client::~Client(){
	notifThread.stop();
	dataThread.stop();
}

void ANCS::Client::onConn(){
	connected = true;
	connect();
}

void ANCS::Client::onDiscon(){
	std::lock_guard lock(needDataMut);
	while(needData.size()){
		needData.pop();
	}
	dataQueue.clear();
	processingAttrs = false;

	connected = false;
	disconnect();
}

void ANCS::Client::actionPos(uint32_t uid){
	std::vector<uint8_t> buf;
	buf.push_back(2);
	buf.insert(buf.cend(), (uint8_t*) &uid, (uint8_t*) (((uint32_t*) &uid) + 1));
	buf.push_back(ActionID::ActionIDPositive);
	ESP_LOGI(TAG, "Sending pos action for notif 0x%lx\n", uid);
	chr.ctrl->write(buf);
}

void ANCS::Client::actionNeg(uint32_t uid){
	std::vector<uint8_t> buf;
	buf.push_back(2);
	buf.insert(buf.cend(), (uint8_t*) &uid, (uint8_t*) (((uint32_t*) &uid) + 1));
	buf.push_back(ActionID::ActionIDNegative);
	ESP_LOGI(TAG, "Sending neg action for notif 0x%lx\n", uid);
	chr.ctrl->write(buf);
}

void ANCS::Client::loopNotif(){
	if(chr.notif == nullptr || !connected){
		vTaskDelay(500);
		return;
	}

	auto notif = chr.notif->getNextNotif();
	if(notif == nullptr) return;

	if(!connected) return;

	auto data = notif->data;
	if(notif->isIndicate){
		ESP_LOGW(TAG, "INDICATE: Notify on notification_source channel. ID: %d, flags: %d, category: %d", data[0], data[1], data[2]);
	}

	EventID evt = (EventID) data[0];
	uint8_t flags = data[1]; // TODO: check if notif has positive/negative action
	CategoryID cat = (CategoryID) data[2];
	uint8_t catCount = data[3];
	uint32_t uid = *((uint32_t*) &data[4]);

	if(evt == NotificationAdded || evt == NotificationModified){
		std::lock_guard lock(needDataMut);
		needData.push(QueuedNotif{ .uid = uid, .category = cat, .modify = (evt == NotificationModified) });
		requestData(uid);
	}else if(evt == NotificationRemoved){
		notifRemove(uid);
	}
}

void ANCS::Client::loopData(){
	if(chr.data == nullptr || !connected){
		vTaskDelay(500);
		return;
	}

	auto notif = chr.data->getNextNotif(processingAttrs ? 3500 : portMAX_DELAY); // if no data for 500ms
	if(notif == nullptr && !processingAttrs) return;

	if(!connected) return;

	if(processingAttrs && notif == nullptr){
		processData(true);
		return;
	}

	dataQueue.insert(dataQueue.cend(), notif->data.cbegin(), notif->data.cend());
	processData(false);
}

void ANCS::Client::requestData(uint32_t uid){
	std::vector<uint8_t> buf;

	buf.push_back(0);
	buf.insert(buf.cend(), (uint8_t*) &uid, (uint8_t*) (((uint32_t*) &uid) + 1));

	for(int i = 0; i < AttributeID::COUNT; i++){
		buf.push_back(i);
		if(AttrNeedLen.count((AttributeID) i)){
			buf.push_back(0xff);
			buf.push_back(0xff);
		}
	}

	chr.ctrl->write(buf);

	ESP_LOGI(TAG, "Requesting data for notif 0x%lx\n", uid);
}

void ANCS::Client::processData(bool sendIncomplete){
	// Send and remove the notification at the front of needData queue
	// Exits the notification processing state (resets processingAttrs)
	auto send = [this](){
		std::unique_lock lock(needDataMut);
		QueuedNotif nd = needData.front(); // important: create a copy, the reference is going to get invalidated after pop below
		needData.pop();
		lock.unlock();

		auto get = [&nd](AttributeID id){
			auto attr = nd.attrs.find(id);
			if(attr == nd.attrs.end()) return std::string();
			return attr->second;
		};

		Notif notif = {
				.uid = nd.uid,
				.title = get(Title),
				//.subtitle = get(Subtitle),
				.message = get(Message),
				.appID = get(AppIdentifier),
				//.time = {}, // TODO
				//.label = { .pos = get(PositiveActionLabel), .neg = get(NegativeActionLabel) },
				.category = (Notif::Category) nd.category // TODO: Currently, Notif categories map 1:1 to ANCS categories. In the future, mapping will be needed
		};

		if(AppIDMap.count(notif.appID)){
			notif.appID = AppIDMap.at(notif.appID);
		}


		ESP_LOGI(TAG, "Sending notif 0x%lx. Modify: %d\n", nd.uid, nd.modify);
		if(nd.modify){
			notifModify(notif);
		}else{
			notifNew(notif);
		}

		processingAttrs = false;
	};

#define err() if(sendIncomplete){ send(); } return

	auto getNext = [this]() -> uint32_t {
		std::lock_guard lock(needDataMut);
		if(needData.size() == 0) return -1;
		return needData.front().uid;
	};

	uint32_t uid = -1;
	while((uid = getNext()) != (uint32_t) -1){
		// Search for start of data for this notification
		if(!processingAttrs){
			uint8_t target[5] = { 0 };
			memcpy(target + 1, &uid, 4);

			int matched = 0, i = 0;
			for(; i < dataQueue.size() && matched < 5; i++){
				if(dataQueue[i] == target[matched]){
					matched++;
				}else{
					matched = 0;

					if(dataQueue[i] == target[matched]){
						matched++;
					}
				}
			}
			dataQueue.erase(dataQueue.begin(), dataQueue.begin() + i - matched);

			if(matched == 5){
				// Matched whole header
				processingAttrs = true;
				ESP_LOGI(TAG, "Found header for notif 0x%lx\n", uid);

				// We've entered the notification processing state, notif at the front of needData is the notif we're processing from now on
				// Erase the found header from data buffer
				dataQueue.erase(dataQueue.begin(), dataQueue.begin() + 5);
			}else{
				// No header found => data is incomplete
				err();
			}
		}

		while(dataQueue.size()){
			std::unique_lock lock(needDataMut);
			auto attrID = needData.front().currAttr;
			auto attrLen = needData.front().currAttrLen;
			lock.unlock();

			// Search for next attribute ID and length
			if(attrID == AttributeID::COUNT){
				if(dataQueue.size() < 3){
					err();
				}

				attrID = (AttributeID) dataQueue[0];
				attrLen = *((uint16_t*) &dataQueue[1]);

				dataQueue.erase(dataQueue.begin(), dataQueue.begin() + 3);

				lock.lock();
				needData.front().currAttr = attrID;
				needData.front().currAttrLen = attrLen;
				lock.unlock();
			}

			// Waiting for rest of attr
			if(dataQueue.size() < attrLen){
				err();
			}

			// Extract attribute
			const auto data = &dataQueue[0];
			const auto val = std::string(data, data + attrLen);
			dataQueue.erase(dataQueue.begin(), dataQueue.begin() + attrLen);

			// Insert new attribute
			lock.lock();
			needData.front().attrs[attrID].append(val);
			needData.front().currAttr = AttributeID::COUNT;
			needData.front().currAttrLen = 0;
			auto attrCount = needData.front().attrs.size();
			lock.unlock();

			// If all attributes received, send the notification
			if(sendIncomplete || attrCount >= AttributeID::COUNT){
				send();
				break;
			}
		}
	}
}
