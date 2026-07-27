#ifndef CLOCKSTAR_FIRMWARE_ANCS_H
#define CLOCKSTAR_FIRMWARE_ANCS_H

#include "BLE/Client.h"
#include "Notifs/NotifSource.h"
#include "Util/Threaded.h"
#include "Util/PSRAMAllocator.h"
#include "Model.h"
#include <queue>
#include <deque>
#include <mutex>
#include <atomic>
#include <cstdint>
#include <vector>
#include <unordered_set>
#include <unordered_map>

namespace ANCS {

class Client : public NotifSource {
public:
	Client(BLE::Client* client);
	virtual ~Client();

	void actionPos(uint32_t uid) override;
	void actionNeg(uint32_t uid) override;

private:
	std::shared_ptr<BLE::Client::Service> service;
	struct {
		std::shared_ptr<BLE::Client::Char> notif;
		std::shared_ptr<BLE::Client::Char> ctrl;
		std::shared_ptr<BLE::Client::Char> data;
	} chr;

	std::atomic<bool> connected{ false };

	void onConn();
	void onDiscon();

	ThreadedClosure notifThread;
	ThreadedClosure dataThread;

	void loopNotif();
	void loopData();

	// Attribute text accumulated off the BLE wire is kept in PSRAM. get() exposes
	// it as std::string_view, so Notif's public std::string members are unaffected.
	using AttrMap = std::unordered_map<AttributeID, PSRAMString, std::hash<AttributeID>, std::equal_to<AttributeID>,
		PSRAMAllocator<std::pair<const AttributeID, PSRAMString>>>;

	struct QueuedNotif {
		uint32_t uid;
		CategoryID category;
		bool modify; // whether it's a new notification or a modification
		AttrMap attrs;
		AttributeID currAttr = AttributeID::COUNT;
		uint32_t currAttrLen = 0;
		AttributeID lastRequested = AttributeID::COUNT; // ID of the last attribute we asked the phone for; ANCS replies are in request order so receiving this means the notif is complete
	};

	// Request pipeline. At most one GetNotificationAttributes is outstanding at
	// a time, so a stale UID produces a single 0xA2 instead of a write storm.
	// loopNotif() only enqueues; loopData() owns the state machine; both may call
	// pumpRequest() (guarded by reqState). A stale/absent UID is dropped either
	// promptly (NotificationRemoved -> abandonInFlight) or, for a true 0xA2 with
	// no removal event, after RequestTimeoutMs elapses with no DataSource reply.
	//
	// mut protects needData, queuedUids, dataQueue and the reqState/inFlight*
	// fields. onDiscon (BLE/BTC task) resets them while the workers mutate them.
	enum class ReqState { Idle, Awaiting, Processing };

	std::deque<QueuedNotif, PSRAMAllocator<QueuedNotif>> needData;
	std::unordered_set<uint32_t> queuedUids;
	std::deque<uint8_t, PSRAMAllocator<uint8_t>> dataQueue;
	std::mutex mut;

	ReqState reqState = ReqState::Idle;
	uint32_t inFlightUid = 0;
	uint64_t reqStartMs = 0;
	bool abandonInFlight = false;

	static constexpr size_t MaxQueued = 16;
	static constexpr uint32_t IdlePollMs = 1000;          // loopData poll while Idle (only to pick up a newly-pumped request)
	static constexpr uint32_t PollMs = 250;               // loopData poll while a request is in flight (Awaiting)
	static constexpr uint64_t RequestTimeoutMs = 3500;    // drop if no DataSource response arrives (e.g. a stale UID / 0xA2)
	static constexpr uint32_t ProcessingTimeoutMs = 3500; // salvage if a notif stalls mid-parse

	void pumpRequest();             // start the next request if Idle; caller holds mut
	void dropInFlight();            // discard the in-flight (front) entry; caller holds mut
	bool requestData(uint32_t uid); // issue GetNotificationAttributes for front; returns whether issued; caller holds mut
	void processData(bool sendIncomplete);

	static constexpr esp_bt_uuid_t ServiceUUID =			{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xD0, 0x00, 0x2D, 0x12, 0x1E, 0x4B, 0x0F, 0xA4, 0x99, 0x4E, 0xCE, 0xB5, 0x31, 0xF4, 0x05, 0x79 }}};
	static constexpr esp_bt_uuid_t Char_NotifSource_UUID =	{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xbd, 0x1d, 0xa2, 0x99, 0xe6, 0x25, 0x58, 0x8c, 0xd9, 0x42, 0x01, 0x63, 0x0d, 0x12, 0xbf, 0x9f }}};
	static constexpr esp_bt_uuid_t Char_ControlPoint_UUID =	{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xd9, 0xd9, 0xaa, 0xfd, 0xbd, 0x9b, 0x21, 0x98, 0xa8, 0x49, 0xe1, 0x45, 0xf3, 0xd8, 0xd1, 0x69 }}};
	static constexpr esp_bt_uuid_t Char_DataSource_UUID =	{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xfb, 0x7b, 0x7c, 0xce, 0x6a, 0xb3, 0x44, 0xbe, 0xb5, 0x4b, 0xd6, 0x24, 0xe9, 0xc6, 0xea, 0x22 }}};

};

}


#endif //CLOCKSTAR_FIRMWARE_ANCS_H
