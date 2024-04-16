#include "Theme5BatteryElement.h"
#include "Util/Services.h"

Theme5BatteryElement::Theme5BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	batteryIndicator = lv_img_create(*this);
	lv_img_set_src(batteryIndicator, THEMED_FILE(LockScreen, BatteryLevel, theme));

	set(getLevel());
}

void Theme5BatteryElement::updateChargingVisuals(){
	lv_obj_set_size(*this, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_top(batteryIndicator, 0, 0);

	if(chargingIndex % 2 == 0){
		lv_obj_clear_flag(batteryIndicator, LV_OBJ_FLAG_HIDDEN);
	}else{
		lv_obj_add_flag(batteryIndicator, LV_OBJ_FLAG_HIDDEN);
	}
}

void Theme5BatteryElement::updateLevelVisuals(){
	const float percent = battery.getPerc() / 100.0f;
	lv_obj_clear_flag(batteryIndicator, LV_OBJ_FLAG_HIDDEN);
	lv_obj_set_size(*this, LV_SIZE_CONTENT, ((lv_img_t*) batteryIndicator)->h);
	lv_obj_set_style_pad_top(batteryIndicator, (1.0f - percent) * ((lv_img_t*) batteryIndicator)->h, 0);
}

void Theme5BatteryElement::loop(){
	BatteryElement::loop();

	if(getLevel() != Charging){
		const float percent = battery.getPerc() / 100.0f;
		lv_obj_clear_flag(batteryIndicator, LV_OBJ_FLAG_HIDDEN);
		lv_obj_set_size(*this, LV_SIZE_CONTENT, ((lv_img_t*) batteryIndicator)->h);
		lv_obj_set_style_pad_top(batteryIndicator, (1.0f - percent) * ((lv_img_t*) batteryIndicator)->h, 0);
	}
}
