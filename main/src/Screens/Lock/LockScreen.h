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
#include "LockSkin.h"
#include "Filepaths.hpp"

// TODO: functionality of this should be separated from UI and this should control UI, which can be different at times but needs same API for interacting with the user through different skins
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

	LockSkin* skin = nullptr;

	/*lv_obj_t* mainTop;

	lv_obj_t* mainMid;
	lv_obj_t* icons;

	lv_obj_t* playing;
	lv_obj_t* lock;

	lv_obj_t* rest;*/

	Time& ts;
	Phone& phone;
	EventQueue queue;

	void onStarting() override;
	void onStart() override;
	void onStop() override;
	void prepare();

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
