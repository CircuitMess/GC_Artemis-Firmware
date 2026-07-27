#ifndef CLOCKSTAR_FIRMWARE_AMS_H
#define CLOCKSTAR_FIRMWARE_AMS_H

#include "BLE/Client.h"
#include "Notifs/MediaSource.h"
#include "Notifs/MediaInfo.h"
#include "Model.h"
#include "Util/Threaded.h"
#include "Util/PSRAMAllocator.h"
#include <atomic>
#include <cstdint>
#include <mutex>
#include <optional>
#include <vector>

namespace AMS {

class Client : public MediaSource {
public:
	Client(BLE::Client* client);
	virtual ~Client();

	void mediaPlay() override;
	void mediaPause() override;
	void mediaNext() override;
	void mediaPrev() override;

private:
	std::shared_ptr<BLE::Client::Service> service;
	struct {
		std::shared_ptr<BLE::Client::Char> remoteCommand;
		std::shared_ptr<BLE::Client::Char> entityUpdate;
	} chr;

	std::atomic<bool> connected{ false };

	void onConn();
	void onDiscon();

	ThreadedClosure updateThread;
	void loopUpdate();

	void sendCommand(RemoteCommandID cmd);
	void registerEntities();
	void processEntityUpdate(const PSRAMByteBuffer& data);
	void flush();

	// mut protects info, hint, state, and the dirty write. onDiscon (BLE event
	// task) resets all of them when a session ends; the update thread mutates
	// them in processEntityUpdate and snapshots info in flush. dirty is read on
	// the update thread outside mut to pick the getNextNotif timeout without a
	// data race, so it's atomic — writes still happen under mut for consistency
	// with the companion state.
	std::mutex mut;

	// Raw aggregated track + appID strings from the source.
	MediaInfo info;
	// Latest PlayerPlaybackInfo intent (Playing or Paused). Reset by
	// processEntityUpdate whenever Track is observed empty so a stale hint
	// from an idle iPhone period can't leak into the next active session.
	// Consumed by flush() to derive the emitted MediaState.
	std::optional<MediaState> hint;
	// Last MediaState emitted; used to suppress duplicate mediaState() calls.
	MediaState state = MediaState::Stopped;
	// Set when any input changes in a way that warrants a flush (info or hint).
	// Cleared by flush() at the start of the emit. Read outside mut to pick the
	// getNextNotif timeout, so it's atomic — writes happen under mut for
	// consistency with the companion state.
	std::atomic<bool> dirty{ false };

	// Coalesce window for batching attribute notifications into a single mediaInfo() / mediaState() emit pair.
	// AMS sends one notification per attribute; bursts during track change / connect arrive within ms,
	// so this only delays the consumer by the window's worth of idle time.
	static constexpr TickType_t BatchTimeout = pdMS_TO_TICKS(500);

	static constexpr esp_bt_uuid_t ServiceUUID =			{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xDC, 0xF8, 0x55, 0xAD, 0x02, 0xC5, 0xF4, 0x8E, 0x3A, 0x43, 0x36, 0x0F, 0x2B, 0x50, 0xD3, 0x89 }}};
	static constexpr esp_bt_uuid_t Char_RemoteCommand_UUID =	{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0xC2, 0x51, 0xCA, 0xF7, 0x56, 0x0E, 0xDF, 0xB8, 0x8A, 0x4A, 0xB1, 0x57, 0xD8, 0x81, 0x3C, 0x9B }}};
	static constexpr esp_bt_uuid_t Char_EntityUpdate_UUID =		{ .len = ESP_UUID_LEN_128, .uuid = { .uuid128 = { 0x02, 0xC1, 0x96, 0xBA, 0x92, 0xBB, 0x0C, 0x9A, 0x1F, 0x41, 0x8D, 0x80, 0xCE, 0xAB, 0x7C, 0x2F }}};

};

}


#endif //CLOCKSTAR_FIRMWARE_AMS_H
