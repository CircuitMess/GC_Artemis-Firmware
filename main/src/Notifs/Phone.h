#ifndef CLOCKSTAR_FIRMWARE_PHONE_H
#define CLOCKSTAR_FIRMWARE_PHONE_H


#include <cstdint>
#include "Bangle.h"
#include "ANCS/Client.h"
#include "CurrentTime.h"
#include "NotifSource.h"
#include <deque>

class Phone {
public:

	enum class PhoneType {
		None, Android, IPhone
	};

	struct Event {
		enum { Connected, Disconnected, Added, Changed, Removed, Cleared } action;
		union {
			struct { uint32_t id; } addChgRem;
			PhoneType phoneType;
		} data;
	};

	static constexpr size_t MaxNotifs = 20;

	Phone(BLE::Server* server, BLE::Client* client);

	bool isConnected();
	PhoneType getPhoneType();

	Notif getNotif(uint32_t uid);
	std::vector<Notif> getNotifs();
	uint32_t getNotifsCount() const;

	void doPos(uint32_t id);
	void doNeg(uint32_t id);

	void findPhoneStart();
	void findPhoneStop();

private:
	ANCS::Client ancs;
	CurrentTime cTime;
	Bangle bangle;

	NotifSource* current = nullptr;

	void onConnect(NotifSource* src);
	void onDisconnect(NotifSource* src);

	void onAdd(Notif notif);
	void onModify(Notif notif);
	void onRemove(uint32_t id);

	// TODO: const size backed dequeue
	// TODO: mutex
	std::deque<Notif> notifs;

	auto findNotif(uint32_t id);

};


#endif //CLOCKSTAR_FIRMWARE_PHONE_H
