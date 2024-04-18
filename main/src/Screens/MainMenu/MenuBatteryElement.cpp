#include "MenuBatteryElement.h"
#include "Util/Services.h"

MenuBatteryElement::MenuBatteryElement(lv_obj_t* parent) : BatteryElement(parent) {
	img = lv_img_create(*this);

	set(getLevel());
}

void MenuBatteryElement::updateChargingVisuals(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	const char* BatteryIcons[] = {
			THEMED_FILE(Menu, BatteryEmpty, theme), THEMED_FILE(Menu, BatteryLow, theme), THEMED_FILE(Menu, BatteryMid, theme), THEMED_FILE(Menu, BatteryFull, theme)
	};

	lv_img_set_src(img, BatteryIcons[chargingIndex]);
}

void MenuBatteryElement::updateLevelVisuals(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	if(theme == Theme::Theme2){
		lv_obj_set_style_pad_top(img, 1, 0);
	}else{
		lv_obj_set_style_pad_top(img, 0, 0);
	}

	const char* BatteryIcons[] = {
			THEMED_FILE(Menu, BatteryEmpty, theme), THEMED_FILE(Menu, BatteryLow, theme), THEMED_FILE(Menu, BatteryMid, theme), THEMED_FILE(Menu, BatteryFull, theme)
	};

	if(getLevel() == Charging){
		lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
		lv_img_set_src(img, BatteryIcons[1]);
	}else{
		lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
		lv_img_set_src(img, BatteryIcons[getLevel()]);
	}
}
