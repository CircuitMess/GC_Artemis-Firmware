#ifndef CLOCKSTAR_FIRMWARE_LOCKSCREEN_H
#define CLOCKSTAR_FIRMWARE_LOCKSCREEN_H

#include "LV_Interface/LVScreen.h"
#include "Item.h"
#include "Util/Events.h"
#include "Notifs/Phone.h"
#include "Services/Time.h"
#include "UIElements/StatusBar.h"
#include "Slider.h"
#include "Devices/Input.h"
#include "UIElements/ClockLabelBig.h"

class LockScreen : public LVScreen {
public:
	LockScreen();

private:

	/** Layout:
	 *
	 * - main
	 * |-- mainTop
	 * |-- mainMid
	 * | |- clock
	 * | |- date
	 * | `- icons
	 * `-- mainBot
	 *
	 * - rest
	 * |-- item
	 * |-- item
	 * `-- item
	 */

	lv_obj_t* main;

	lv_obj_t* mainTop;

	lv_obj_t* mainMid;
	ClockLabelBig* clock;
	lv_obj_t* icons;

	Slider* locker;
	lv_obj_t* playing;
	lv_obj_t* lock;

	lv_obj_t* rest;

	StatusBar* status;

	Time& ts;
	Phone& phone;
	EventQueue queue;

	static constexpr uint8_t MaxNotifs = 20;
	std::unordered_map<uint32_t, Item*> notifs;

	struct NotifIcon {
		uint32_t count;
		lv_obj_t* icon;
	};
	std::unordered_map<const char*, NotifIcon> notifIcons;
	static constexpr uint8_t MaxIconsCount = MaxNotifs;
	static constexpr const char* EtcIconPath = "S:/icon/etc.bin";

	void onStarting() override;
	void onStart() override;
	void onStop() override;
	void prepare();

	void updateNotifs();
	void notifAdd(const Notif& notif);
	void notifRem(uint32_t id);
	void notifsClear();

	void addNotifIcon(const char* path);
	void removeNotifIcon(const char* path);

	void loop() override;
	void processEvt(const Phone::Event& evt);
	void processInput(const Input::Data& evt);

	void updateTime(const tm& time);
	static constexpr uint32_t TimeUpdateInterval = 200;
	uint64_t lastTimeUpdate = 0;

	void buildUI();

	void setSleep(bool sleep);

};


#endif //CLOCKSTAR_FIRMWARE_LOCKSCREEN_H
