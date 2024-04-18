#include "Theme4BatteryElement.h"
#include "Util/Services.h"

Theme4BatteryElement::Theme4BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(*this, 2, 0);

	clockIcon = lv_img_create(*this);
	lv_img_set_src(clockIcon, THEMED_FILE(LockScreen, Clock, theme));

	batteryIcon = lv_img_create(*this);
	lv_obj_set_style_pad_top(batteryIcon, 1, 0);

	set(getLevel());
}

void Theme4BatteryElement::updateChargingVisuals(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	const char* BatteryIcons[] = {
			THEMED_FILE(LockScreen, Battery1, theme), THEMED_FILE(LockScreen, Battery2, theme), THEMED_FILE(LockScreen, Battery3, theme), THEMED_FILE(LockScreen, Battery4, theme)
	};

	lv_img_set_src(batteryIcon, BatteryIcons[chargingIndex]);
}

void Theme4BatteryElement::updateLevelVisuals(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	const char* BatteryIcons[] = {
			THEMED_FILE(LockScreen, Battery1, theme), THEMED_FILE(LockScreen, Battery2, theme), THEMED_FILE(LockScreen, Battery3, theme), THEMED_FILE(LockScreen, Battery4, theme)
	};

	if(getLevel() == Charging){
		lv_img_set_src(batteryIcon, BatteryIcons[1]);
	}else{
		lv_img_set_src(batteryIcon, BatteryIcons[getLevel()]);
	}
}
