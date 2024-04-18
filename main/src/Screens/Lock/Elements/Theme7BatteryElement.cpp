#include "Theme7BatteryElement.h"
#include "Util/Services.h"

Theme7BatteryElement::Theme7BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	batteryLevels = 5;

	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(*this, 1, 0);

	lvl1 = lv_img_create(*this);
	lv_img_set_src(lvl1, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl2 = lv_img_create(*this);
	lv_img_set_src(lvl2, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl3 = lv_img_create(*this);
	lv_img_set_src(lvl3, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl4 = lv_img_create(*this);
	lv_img_set_src(lvl4, THEMED_FILE(LockScreen, BatteryLevel, theme));

	set(getLevel());
}

void Theme7BatteryElement::updateChargingVisuals(){
	lv_obj_t* const levels[] = { lvl1, lvl2, lvl3, lvl4 };

	for(size_t i = 0; i < chargingIndex; ++i){
		lv_obj_clear_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
	}

	for(size_t i = chargingIndex; i < batteryLevels - 1; ++i){
		lv_obj_add_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
	}
}

void Theme7BatteryElement::updateLevelVisuals(){
	if(getLevel() == Charging){
		lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl4, LV_OBJ_FLAG_HIDDEN);

		return;
	}

	const Battery::Level level = battery.getLevel();

	if(level > Battery::Critical){
		lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::VeryLow){
		lv_obj_clear_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::Low){
		lv_obj_clear_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::Mid){
		lv_obj_clear_flag(lvl4, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Full){
		lv_obj_add_flag(lvl4, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Mid){
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Low){
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::VeryLow){
		lv_obj_add_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}
}

void Theme7BatteryElement::loop(){
	BatteryElement::loop();

	if(getLevel() == Charging){
		return;
	}

	const Battery::Level level = battery.getLevel();

	if(level > Battery::Critical){
		lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::VeryLow){
		lv_obj_clear_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::Low){
		lv_obj_clear_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(level > Battery::Mid){
		lv_obj_clear_flag(lvl4, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Full){
		lv_obj_add_flag(lvl4, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Mid){
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::Low){
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(level < Battery::VeryLow){
		lv_obj_add_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}
}
