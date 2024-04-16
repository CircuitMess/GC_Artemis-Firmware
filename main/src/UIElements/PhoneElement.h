#ifndef ARTEMIS_FIRMWARE_PHONEELEMENT_H
#define ARTEMIS_FIRMWARE_PHONEELEMENT_H

#include "LV_Interface/LVObject.h"
#include "Notifs/Phone.h"

class PhoneElement : public LVObject {
public:
	explicit PhoneElement(lv_obj_t* parent, bool showNotifIcon = true, bool lockScreen = false);

	void loop();

	virtual void updateVisuals() override;

private:
	const bool showNotifIcon;
	Phone& phone;
	bool connected = false;
	bool notifPresent = false;
	bool lockScreen;

	lv_obj_t* phoneIcon;
	lv_obj_t* notifIcon;

	const char* phonePath;
	const char* phoneDcPath;
	const char* notifPath;

	void setPhoneConnected();
	void setNotifIcon();
};


#endif //ARTEMIS_FIRMWARE_PHONEELEMENT_H
