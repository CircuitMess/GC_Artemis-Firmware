#ifndef CLOCKSTAR_FIRMWARE_STATUSBAR_H
#define CLOCKSTAR_FIRMWARE_STATUSBAR_H

#include "LV_Interface/LVObject.h"
#include "ClockLabelSmall.h"
#include "Services/Time.h"
#include "Notifs/Phone.h"
#include "Util/Events.h"
#include "Devices/Battery.h"
#include "BatteryElement.h"
#include "PhoneElement.h"

class StatusBar : public LVObject {
public:
	explicit StatusBar(lv_obj_t* parent, bool showExtra = true);

	void loop();

private:
	bool showExtra = false;

	lv_obj_t* left;
	lv_obj_t* right;

	ClockLabelSmall* clock = nullptr;
	BatteryElement* batDevice = nullptr;
	PhoneElement* phone = nullptr;

	void buildUI();
};

#endif //CLOCKSTAR_FIRMWARE_STATUSBAR_H
