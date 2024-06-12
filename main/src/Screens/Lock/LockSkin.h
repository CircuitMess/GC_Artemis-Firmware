#ifndef ARTEMIS_FIRMWARE_LOCKSKIN_H
#define ARTEMIS_FIRMWARE_LOCKSKIN_H

#include "LV_Interface/LVObject.h"
#include "UIElements/StatusBar.h"
#include "UIElements/ClockLabelBig.h"
#include "Slider.h"
#include "Item.h"

class LockSkin : public LVObject {
public:
	LockSkin(lv_obj_t* parent, 	lv_group_t* inputGroup);

	inline Slider* getLocker() const { return locker; }
	inline lv_obj_t* getMain() noexcept { return main; }

	void loop();
	void prepare();

	void notifAdd(const Notif& notif);
	void notifRem(uint32_t id);
	void notifsClear();

private:
	lv_group_t* inputGroup = nullptr;
	lv_obj_t* main = nullptr;
	PhoneElement* phoneElement;
	BatteryElement* batteryElement;
	lv_obj_t* battery;
	ClockLabelBig* clock = nullptr;
	class NotifIconsElement* icons = nullptr;
	Slider* locker = nullptr;
	lv_obj_t* rest = nullptr;
	lv_obj_t* notifList = nullptr;
	lv_obj_t* date = nullptr;

	static constexpr uint8_t MaxNotifs = 20;
	std::unordered_map<uint32_t, Item*> notifs;

private:
	void addNotifIcon(NotifIcon icon);
	void removeNotifIcon(NotifIcon icon);
	void updateNotifs();
	void buildUI();
	void setDateLabel();
};

#endif //ARTEMIS_FIRMWARE_LOCKSKIN_H