#include "Theme8BatteryElement.h"
#include "Util/Services.h"

Theme8BatteryElement::Theme8BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	batteryLevels = 7;

	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(*this, 0, 0);

	lvl1 = lv_img_create(*this);
	lv_img_set_src(lvl1, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl2 = lv_img_create(*this);
	lv_img_set_src(lvl2, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl3 = lv_img_create(*this);
	lv_img_set_src(lvl3, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl4 = lv_img_create(*this);
	lv_img_set_src(lvl4, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl5 = lv_img_create(*this);
	lv_img_set_src(lvl5, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lvl6 = lv_img_create(*this);
	lv_img_set_src(lvl6, THEMED_FILE(LockScreen, BatteryLevel, theme));

	set(getLevel());
}

void Theme8BatteryElement::updateChargingVisuals(){
	lv_obj_t* const levels[] = { lvl1, lvl2, lvl3, lvl4, lvl5, lvl6 };

	for(size_t i = 0; i < chargingIndex; ++i){
		lv_obj_clear_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
	}

	for(size_t i = chargingIndex; i < batteryLevels - 1; ++i){
		lv_obj_add_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
	}
}

void Theme8BatteryElement::updateLevelVisuals(){
	if(getLevel() == Charging){
		lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl4, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl5, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl6, LV_OBJ_FLAG_HIDDEN);

		return;
	}

	const uint8_t percent = battery.getPerc();
	const float stepFactor = 100.0f / batteryLevels;
	lv_obj_t* const levels[] = { lvl1, lvl2, lvl3, lvl4, lvl5, lvl6 };

	for(size_t i = 0; i < batteryLevels - 1; ++i){
		if(percent >= (i + 1) * stepFactor){
			lv_obj_clear_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
		}else{
			lv_obj_add_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}

void Theme8BatteryElement::loop(){
	BatteryElement::loop();

	if(getLevel() == Charging){
		return;
	}

	const uint8_t percent = battery.getPerc();
	const float stepFactor = 100.0f / batteryLevels;
	lv_obj_t* const levels[] = { lvl1, lvl2, lvl3, lvl4, lvl5, lvl6 };

	for(size_t i = 0; i < batteryLevels - 1; ++i){
		if(percent >= (i + 1) * stepFactor){
			lv_obj_clear_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
		}else{
			lv_obj_add_flag(levels[i], LV_OBJ_FLAG_HIDDEN);
		}
	}
}
