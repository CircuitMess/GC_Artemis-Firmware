#include "BatteryElement.h"
#include "Util/stdafx.h"

BatteryElement::BatteryElement(lv_obj_t* parent) : LVObject(parent){
	lv_obj_set_size(*this, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	img = lv_img_create(*this);
}

void BatteryElement::set(BatteryElement::Level level){
	this->level = level;
	if(level == Charging){
		chargingMillis = millis();
		chargingIndex = 0;
		lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
		lv_img_set_src(img, "S:/icons/batteryLow.bin");
	}else{
		lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
		lv_img_set_src(img, BatteryIcons[level]);
	}
}

void BatteryElement::loop(){
	if(level == Charging){
		if(millis() - chargingMillis > ChargingAnimTime){
			chargingIndex = (chargingIndex + 1) % BatteryLevels;
			chargingMillis = millis();
			lv_img_set_src(img, BatteryIcons[chargingIndex]);
		}
	}
}

BatteryElement::Level BatteryElement::getLevel() const{
	return level;
}
