#ifndef CLOCKSTAR_FIRMWARE_SLIDER_H
#define CLOCKSTAR_FIRMWARE_SLIDER_H


#include "LV_Interface/LVObject.h"

class Slider : public LVObject {
public:
	Slider(lv_obj_t* parent);

	void loop();
	void start();
	void stop();

	bool started();
	float t();

	void activity();
	void hide();

private:
	lv_obj_t* icon;

	static constexpr uint32_t Duration = 500; // [ms]
	uint64_t startTime = 0;

	static constexpr uint32_t InactivityTimeout = 1000; // [ms]
	uint32_t activityTime = 0;

	static constexpr uint32_t LockY = 3;

};


#endif //CLOCKSTAR_FIRMWARE_SLIDER_H
