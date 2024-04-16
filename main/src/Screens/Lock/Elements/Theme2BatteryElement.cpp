#include "Theme2BatteryElement.h"
#include "Util/Services.h"

Theme2BatteryElement::Theme2BatteryElement(lv_obj_t* parent) : BatteryElement(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	main = lv_obj_create(*this);
	lv_obj_set_size(main, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(main, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(main, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_style_pad_gap(main, 2, 0);

	clockImg = lv_img_create(main);
	lvl1 = lv_img_create(main);
	lvl2 = lv_img_create(main);
	lvl3 = lv_img_create(main);

	lv_img_set_src(clockImg, THEMED_FILE(LockScreen, Clock, theme));
	lv_img_set_src(lvl1, THEMED_FILE(LockScreen, BatteryLevel, theme));
	lv_img_set_src(lvl2, THEMED_FILE(LockScreen, BatteryLevel, theme));
	lv_img_set_src(lvl3, THEMED_FILE(LockScreen, BatteryLevel, theme));

	lv_obj_set_style_pad_top(lvl1, 1, 0);
	lv_obj_set_style_pad_top(lvl2, 1, 0);
	lv_obj_set_style_pad_top(lvl3, 1, 0);

	set(getLevel());
}

void Theme2BatteryElement::updateChargingVisuals(){
	if(chargingIndex > 0){
		lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}

	if(chargingIndex > 1){
		lv_obj_clear_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(chargingIndex > 2){
		lv_obj_clear_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(chargingIndex < 3){
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}

	if(chargingIndex < 2){
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
	}

	if(chargingIndex < 1){
		lv_obj_add_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
	}
}

void Theme2BatteryElement::updateLevelVisuals(){
	const Level level = getLevel();

	if(level == Charging){
		lv_obj_add_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
		lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
	}else{
		if(level > Empty){
			lv_obj_clear_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
		}

		if(level > Low){
			lv_obj_clear_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
		}

		if(level > Mid){
			lv_obj_clear_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
		}

		if(level < Full){
			lv_obj_add_flag(lvl3, LV_OBJ_FLAG_HIDDEN);
		}

		if(level < Mid){
			lv_obj_add_flag(lvl2, LV_OBJ_FLAG_HIDDEN);
		}

		if(level < Low){
			lv_obj_add_flag(lvl1, LV_OBJ_FLAG_HIDDEN);
		}
	}
}
