#ifndef ARTEMIS_FIRMWARE_CALLSCREEN_H
#define ARTEMIS_FIRMWARE_CALLSCREEN_H

#include "Notifs/Notif.h"
#include "LV_Interface/LVScreen.h"
#include "LV_Interface/LVStyle.h"
#include "Util/Events.h"

class CallScreen : public LVScreen {
public:
	CallScreen();
	~CallScreen() override;

private:
	LVStyle textStyle;
	LVStyle btnStyle;
	LVStyle btnStyleFocus;
	LVStyle btnStylePress;

	Notif notif;

	void onIgnore();
	void onReject();

	static constexpr uint32_t NotifInterval = 1000;
	uint32_t notifTime;
	void onStart() override;

	EventQueue evts;
	bool altPress = false;
	void loop() override;

	void buildUI();

};


#endif //ARTEMIS_FIRMWARE_CALLSCREEN_H
