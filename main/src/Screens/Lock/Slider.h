#ifndef CLOCKSTAR_FIRMWARE_SLIDER_H
#define CLOCKSTAR_FIRMWARE_SLIDER_H

#include "LV_Interface/LVObject.h"

struct SliderConfig {
	lv_coord_t start = 0;
	lv_coord_t end = 0;
	lv_coord_t y = 0;
	bool neverHide = false;
};

class Slider : public LVObject {
public:
	explicit Slider(lv_obj_t* parent, SliderConfig config = {});

	void loop();
	void start();
	void stop();

	bool started();
	float t();

	void activity();
	void hide();

private:
	lv_obj_t* icon;

	uint64_t startTime = 0;

	static constexpr uint32_t Duration = 500;
	static constexpr uint32_t InactivityTimeout = 1000; // [ms]
	uint32_t activityTime = 0;

	SliderConfig config;
};


#endif //CLOCKSTAR_FIRMWARE_SLIDER_H
