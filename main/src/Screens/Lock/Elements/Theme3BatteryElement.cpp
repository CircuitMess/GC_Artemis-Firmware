#include "Theme3BatteryElement.h"
#include "Util/Services.h"
#include "Theme/theme.h"

Theme3BatteryElement::Theme3BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(*this, 2, 0);

	batteryImg = lv_img_create(*this);
	lv_img_set_src(batteryImg, THEMED_FILE(LockScreen, BatteryLevel, theme));
	lv_obj_set_style_pad_top(batteryImg, 1, 0);

	batteryPercent = lv_label_create(*this);
	lv_obj_set_style_text_font(batteryPercent, &landerfont, 0);
	lv_obj_set_style_text_color(batteryPercent, lv_color_white(), 0);
	lv_obj_add_flag(batteryPercent, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_pad_top(batteryPercent, 1, 0);

	chargingImg = lv_img_create(*this);
	lv_img_set_src(chargingImg, THEMED_FILE(LockScreen, Charging, theme));
	lv_obj_set_style_align(chargingImg, LV_ALIGN_CENTER, 0);
	lv_obj_add_flag(chargingImg, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_style_pad_left(chargingImg, 6, 0);

	set(getLevel());
}

void Theme3BatteryElement::updateChargingVisuals(){
	if(chargingIndex % 2 == 0){
		lv_obj_add_flag(chargingImg, LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_clear_flag(chargingImg, LV_OBJ_FLAG_HIDDEN);
	}

	lv_obj_add_flag(batteryPercent, LV_OBJ_FLAG_HIDDEN);
}

void Theme3BatteryElement::updateLevelVisuals(){
	lv_obj_add_flag(chargingImg, LV_OBJ_FLAG_HIDDEN);
	lv_obj_clear_flag(batteryPercent, LV_OBJ_FLAG_HIDDEN);
	lv_label_set_text(batteryPercent, std::to_string(battery.getPerc()).c_str());
}

void Theme3BatteryElement::loop(){
	BatteryElement::loop();

	if(getLevel() != Charging){
		lv_obj_clear_flag(batteryPercent, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(chargingImg, LV_OBJ_FLAG_HIDDEN);
		lv_label_set_text(batteryPercent, std::to_string(battery.getPerc()).c_str());
	}
}
