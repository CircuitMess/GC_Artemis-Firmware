#include "Client.h"
#include "Util/stdafx.h"
#include "Util/TextSanitize.h"
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
	{
		std::lock_guard lock(mut);
		needData.clear();
		queuedUids.clear();
		dataQueue.clear();
		reqState = ReqState::Idle;
		inFlightUid = 0;
		abandonInFlight = false;
	}

	connected = false;
	disconnect();
}

void ANCS::Client::actionPos(uint32_t uid){
	std::vector<uint8_t> buf;
	buf.push_back(CommandID::PerformNotificationAction);
	uint8_t uidBytes[4];
	memcpy(uidBytes, &uid, 4);
	buf.insert(buf.cend(), uidBytes, uidBytes + 4);
	buf.push_back(ActionID::ActionIDPositive);
	ESP_LOGI(TAG, "Sending pos action for notif 0x%lx\n", uid);
	chr.ctrl->write(buf);
}

void ANCS::Client::actionNeg(uint32_t uid){
	std::vector<uint8_t> buf;
	buf.push_back(CommandID::PerformNotificationAction);
	uint8_t uidBytes[4];
	memcpy(uidBytes, &uid, 4);
	buf.insert(buf.cend(), uidBytes, uidBytes + 4);
	buf.push_back(ActionID::ActionIDNegative);
	ESP_LOGI(TAG, "Sending neg action for notif 0x%lx\n", uid);
	chr.ctrl->write(buf);
}

void ANCS::Client::loopNotif(){
	if(chr.notif == nullptr || !connected){
		delayMillis(1000);
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
	uint32_t uid;
	memcpy(&uid, &data[4], 4);

	if(evt == NotificationAdded || evt == NotificationModified){
		const bool modify = (evt == NotificationModified);

		std::lock_guard lock(mut);

		// De-dup: iOS re-sends NotificationModified for the same UID repeatedly
		// (timers, navigation, now-playing). Collapse onto the existing entry
		// instead of queueing and requesting it again.
		if(queuedUids.count(uid)){
			for(auto& q : needData){
				if(q.uid == uid){
					q.modify = q.modify || modify;
					break;
				}
			}
			return;
		}

		// Bound growth so a flood of distinct UIDs can't exhaust the heap.
		if(needData.size() >= MaxQueued){
			for(auto it = needData.begin(); it != needData.end(); ++it){
				// Protect the in-flight entry only while a request is actually
				// in flight; gating on reqState (rather than the inFlightUid
				// value) avoids a collision should a real NotificationUID be 0.
				if(reqState != ReqState::Idle && it->uid == inFlightUid) continue;
				ESP_LOGW(TAG, "needData full; dropping queued notif 0x%lx", it->uid);
				queuedUids.erase(it->uid);
				needData.erase(it);
				break;
			}
		}

		needData.push_back(QueuedNotif{ .uid = uid, .category = cat, .modify = modify });
		queuedUids.insert(uid);
		pumpRequest(); // issue now if the pipeline is idle (no added latency)
	}else if(evt == NotificationRemoved){
		{
			std::lock_guard lock(mut);
			if(queuedUids.count(uid)){
				if(reqState != ReqState::Idle && uid == inFlightUid){
					// Currently fetching it; let loopData drop it without emitting.
					abandonInFlight = true;
				}else{
					for(auto it = needData.begin(); it != needData.end(); ++it){
						if(it->uid == uid){
							needData.erase(it);
							break;
						}
					}
					queuedUids.erase(uid);
				}
			}
		}
		notifRemove(uid);
	}
}

void ANCS::Client::loopData(){
	if(chr.data == nullptr || !connected){
		delayMillis(1000);
		return;
	}

	// Pick the block duration from the request state and (re)start the next
	// request if the pipeline is idle. Held briefly; mut is released before the
	// blocking getNextNotif() below.
	//   - Idle + empty queue: long idle poll. There's nothing to do but
	//     periodically pick up a request loopNotif pumped while we were parked;
	//     a successful request's DataSource data interrupts the wait anyway, so
	//     this only bounds how soon a no-reply (stale-UID) request starts timing
	//     out. portMAX_DELAY can't be used here: a failed request that produces
	//     no DataSource bytes would never wake us.
	//   - Awaiting (request in flight): short poll to enforce RequestTimeoutMs.
	//   - Processing (mid-parse): wait up to ProcessingTimeoutMs for the next chunk.
	TickType_t wait;
	{
		std::lock_guard lock(mut);
		pumpRequest();
		switch(reqState){
			case ReqState::Processing: wait = pdMS_TO_TICKS(ProcessingTimeoutMs); break;
			case ReqState::Awaiting:   wait = pdMS_TO_TICKS(PollMs); break;
			default:                   wait = pdMS_TO_TICKS(needData.empty() ? IdlePollMs : PollMs); break;
		}
	}

	auto notif = chr.data->getNextNotif(wait);
	if(!connected) return;

	std::lock_guard lock(mut);

	// 1) The in-flight notification was removed mid-fetch: drop it silently.
	if(abandonInFlight && reqState != ReqState::Idle){
		dropInFlight();
		return;
	}

	// 2) Timeout (no DataSource bytes this cycle).
	if(notif == nullptr){
		if(reqState == ReqState::Processing){
			processData(true); // mid-notif stall: emit whatever attributes we have
		}else if(reqState == ReqState::Awaiting && (millis() - reqStartMs) >= RequestTimeoutMs){
			// No DataSource reply at all (e.g. a stale UID the phone rejected with
			// 0xA2). Drop it and let the pump move on.
			ESP_LOGW(TAG, "No DataSource response for 0x%lx; dropping", inFlightUid);
			dropInFlight();
		}
		return;
	}

	// 3) DataSource bytes arrived for the in-flight notification.
	dataQueue.insert(dataQueue.cend(), notif->data.cbegin(), notif->data.cend());
	processData(false);
}

void ANCS::Client::pumpRequest(){
	// Caller holds mut. Issue the next GetNotificationAttributes iff nothing is
	// in flight, so a single request is outstanding at a time.
	if(reqState != ReqState::Idle) return;
	if(needData.empty()) return;

	const uint32_t uid = needData.front().uid;

	if(!requestData(uid)){
		// Couldn't issue (e.g. characteristic not ready yet); retry next cycle.
		return;
	}

	inFlightUid = uid;
	reqStartMs = millis();
	reqState = ReqState::Awaiting;
}

void ANCS::Client::dropInFlight(){
	// Caller holds mut. Discard the in-flight (front) entry and reset to Idle.
	if(!needData.empty()){
		queuedUids.erase(needData.front().uid);
		needData.pop_front();
	}
	dataQueue.clear();
	reqState = ReqState::Idle;
	inFlightUid = 0;
	abandonInFlight = false;
}

bool ANCS::Client::requestData(uint32_t uid){
	// Caller holds mut. Issues GetNotificationAttributes for the front entry
	// (the one processData will parse). Returns whether the write went out.
	if(needData.empty()){
		ESP_LOGW(TAG, "requestData(0x%lx) called with empty needData; skipping", uid);
		return false;
	}

	std::vector<uint8_t> buf;

	buf.push_back(CommandID::GetNotificationAttributes);
	uint8_t uidBytes[4];
	memcpy(uidBytes, &uid, 4);
	buf.insert(buf.cend(), uidBytes, uidBytes + 4);

	// Only request attributes the consumer actually uses; the others are commented
	// out so they can be returned to the list cheaply if/when needed.
	constexpr AttributeID requested[] = {
			AttributeID::AppIdentifier,
			AttributeID::Title,
			//AttributeID::Subtitle,
			AttributeID::Message,
			//AttributeID::MessageSize,
			//AttributeID::Date,
			//AttributeID::PositiveActionLabel,
			//AttributeID::NegativeActionLabel,
	};

	AttributeID lastRequested = AttributeID::COUNT;
	for(AttributeID id : requested){
		buf.push_back(id);
		if(AttrNeedLen.count(id)){
			buf.push_back(0x00); // max attribute length = 256 (0x0100), little-endian
			buf.push_back(0x01);
		}
		lastRequested = id;
	}
	needData.front().lastRequested = lastRequested;

	if(!chr.ctrl->write(buf)) return false;

	ESP_LOGI(TAG, "Requesting data for notif 0x%lx\n", uid);
	return true;
}

void ANCS::Client::processData(bool sendIncomplete){
	// Caller holds mut. Single-flight: needData.front() is the in-flight notif
	// and dataQueue only ever holds that notif's DataSource bytes.
	if(needData.empty()){
		dataQueue.clear();
		reqState = ReqState::Idle;
		inFlightUid = 0;
		abandonInFlight = false;
		return;
	}

	// Emit the front notification and reset to Idle. Honors abandonInFlight
	// (notification was removed mid-fetch) by dropping it without surfacing it.
	auto send = [this](){
		QueuedNotif nd = needData.front(); // copy; the reference dies on pop_front below
		needData.pop_front();
		queuedUids.erase(nd.uid);

		// loopData drops a removed in-flight notification (abandonInFlight) before
		// processData runs, so it is always false here; just reset the state.
		reqState = ReqState::Idle;
		inFlightUid = 0;
		abandonInFlight = false;

		auto get = [&nd](AttributeID id) -> std::string_view {
			auto attr = nd.attrs.find(id);
			if(attr == nd.attrs.end()) return {};
			return attr->second;
		};

		Notif notif = {
				.uid = nd.uid,
				.title = sanitizeToAscii(get(Title)),
				//.subtitle = get(Subtitle),
				.message = sanitizeToAscii(get(Message)),
				.appID = std::string(get(AppIdentifier)),
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
	};

	// Mid-notif timeout: emit whatever fully-parsed attributes we collected.
	if(sendIncomplete){
		send();
		return;
	}

	const uint32_t uid = needData.front().uid;

	// Search for the start of this notification's attribute data if not parsing yet.
	if(reqState != ReqState::Processing){
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
			// Matched whole header; enter the parsing state and erase the header.
			reqState = ReqState::Processing;
			ESP_LOGI(TAG, "Found header for notif 0x%lx\n", uid);
			dataQueue.erase(dataQueue.begin(), dataQueue.begin() + 5);
		}else{
			// No header yet => wait for more DataSource bytes.
			return;
		}
	}

	while(dataQueue.size()){
		auto attrID = needData.front().currAttr;
		auto attrLen = needData.front().currAttrLen;

		// Search for next attribute ID and length
		if(attrID == AttributeID::COUNT){
			if(dataQueue.size() < 3){
				return;
			}

			attrID = (AttributeID) dataQueue[0];
			attrLen = (uint16_t) dataQueue[1] | ((uint16_t) dataQueue[2] << 8);

			dataQueue.erase(dataQueue.begin(), dataQueue.begin() + 3);

			needData.front().currAttr = attrID;
			needData.front().currAttrLen = attrLen;
		}

		// Waiting for rest of attr
		if(dataQueue.size() < attrLen){
			return;
		}

		// Extract attribute directly into the PSRAM-backed attr string (std::deque
		// is not contiguous; assemble byte-by-byte).
		auto& dst = needData.front().attrs[attrID];
		dst.reserve(dst.size() + attrLen);
		for(uint32_t k = 0; k < attrLen; k++){
			dst.push_back((char) dataQueue[k]);
		}
		dataQueue.erase(dataQueue.begin(), dataQueue.begin() + attrLen);
		needData.front().currAttr = AttributeID::COUNT;
		needData.front().currAttrLen = 0;
		auto lastRequested = needData.front().lastRequested;

		// ANCS replies are in request order; receiving the last requested attribute means the notif is complete
		if(attrID == lastRequested){
			send();
			break;
		}
	}
}
