#ifndef CLOCKSTAR_FIRMWARE_PHONE_H
#define CLOCKSTAR_FIRMWARE_PHONE_H

#include "Android.h"
#include "ANCS/Client.h"
#include "AMS/Client.h"
#include "CurrentTime.h"
#include "NotifSource.h"
#include "MediaSource.h"
#include <deque>
#include <cstdint>
#include <optional>

class Phone {
public:

	enum class PhoneType {
		None, Android, IPhone
	};

	struct Event {
		enum { Connected, Disconnected, Added, Changed, Removed, Cleared,
		      MediaConnected, MediaDisconnected, MediaState, MediaInfo, BatteryChanged } action;
		union {
			struct { uint32_t id; } addChgRem;
			PhoneType phoneType;
			::MediaState mediaState;
			uint8_t battery;
		} data;
	};

	static constexpr size_t MaxNotifs = 20;

	Phone(BLE::Server* server, BLE::Client* client);

	bool isConnected();
	PhoneType getPhoneType();

	Notif getNotif(uint32_t uid);
	std::vector<Notif> getNotifs();
	uint32_t getNotifsCount() const;

	Notif getCall();
	void callIgnore(uint32_t uid);
	void callReject(uint32_t uid);
	void callAnswer(uint32_t uid);

	std::optional<uint8_t> getPhoneBattery() const;

	void doPos(uint32_t id);
	void doNeg(uint32_t id);

	// Media controls
	void doMediaPlay();
	void doMediaPause();
	void doMediaNext();
	void doMediaPrev();

	const MediaInfo& getMedia() const;
	MediaState getMediaState();

	void findPhoneStart();
	void findPhoneStop();
	bool findPhoneActive();

	Android& getAndroid();

private:
	ANCS::Client ancs;
	AMS::Client ams;
	CurrentTime cTime;
	Android android;

	NotifSource* current = nullptr;
	MediaSource* mediaCurrent = nullptr;

	MediaInfo currentMedia;
	MediaState currentMediaState = MediaState::Stopped;

	std::optional<uint8_t> phoneBattery;

	void onConnect(NotifSource* src);
	void onDisconnect(NotifSource* src);

	void onMediaConnect(MediaSource* src);
	void onMediaDisconnect(MediaSource* src);

	void onMediaInfo(const MediaInfo& media);
	void onMediaState(MediaState state);

	void onBattery(uint8_t percent);

	void onAdd(Notif notif);
	void onModify(Notif notif);
	void onRemove(uint32_t id);

	// TODO: const size backed dequeue
	// TODO: mutex
	std::deque<Notif> notifs;

	auto findNotif(uint32_t id);
};


#endif //CLOCKSTAR_FIRMWARE_PHONE_H
