#ifndef CLOCKSTAR_FIRMWARE_STATUSBAR_H
#define CLOCKSTAR_FIRMWARE_STATUSBAR_H

#include "LV_Interface/LVObject.h"
#include "ClockLabelSmall.h"
#include "Services/Time.h"
#include "Notifs/Phone.h"
#include "Util/Events.h"
#include "Devices/Battery.h"
#include "Screens/MainMenu/MenuBatteryElement.h"
#include "PhoneElement.h"

class StatusBar : public LVObject {
public:
	explicit StatusBar(lv_obj_t* parent, bool showExtra = true);

	void loop();

	void set24hFormat(bool format);

	virtual void updateVisuals() noexcept;

private:
	bool showExtra = false;

	lv_obj_t* left;
	lv_obj_t* right;

	ClockLabelSmall* clock = nullptr;
	MenuBatteryElement* batDevice = nullptr;
	PhoneElement* phone = nullptr;

	void buildUI();
};

#endif //CLOCKSTAR_FIRMWARE_STATUSBAR_H
