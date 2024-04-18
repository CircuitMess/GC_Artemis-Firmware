#include <cmath>
#include "Slider.h"
#include "Util/stdafx.h"
#include "Filepaths.hpp"
#include "Settings/Settings.h"
#include "Util/Services.h"

Slider::Slider(lv_obj_t* parent, SliderConfig config) : LVObject(parent), config(config){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	lv_obj_set_size(*this, 128, 16);

	icon = lv_img_create(*this);
	lv_img_set_src(icon, THEMED_FILE(Icons, LockClosed, theme));

	if(config.start > config.end){
		lv_img_t* img = (lv_img_t*) icon;
		lv_obj_set_pos(icon, config.start - img->w, config.y);
	}else{
		lv_obj_set_pos(icon, config.start, config.y);
	}
}

bool Slider::started(){
	return startTime != 0;
}

float Slider::t(){
	if(startTime == 0) return 0;
	return (float) (millis() - startTime) / (float) Duration;
}

void Slider::loop(){
	if(startTime == 0 && activityTime == 0) return;

	if(startTime != 0){
		if(config.start > config.end){
			lv_img_t* img = (lv_img_t*) icon;
			lv_obj_set_pos(icon, std::max((int16_t) (config.start - img->w - std::round((config.start - config.end) * t())), config.end), config.y);
		}else{
			lv_obj_set_pos(icon, std::min((int16_t) (config.start + std::round((config.end - config.start) * t())), config.end), config.y);
		}

		return;
	}

	if(millis() - activityTime >= InactivityTimeout){
		hide();
		if(config.start > config.end){
			lv_img_t* img = (lv_img_t*) icon;
			lv_obj_set_pos(icon, config.start - img->w, config.y);
		}else{
			lv_obj_set_pos(icon, config.start, config.y);
		}
	}
}

void Slider::start(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	if(startTime != 0) return;

	startTime = millis();
	lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
	lv_img_set_src(icon, THEMED_FILE(Icons, LockOpen, theme));
}

void Slider::stop(){
	if(startTime == 0) return;

	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	const Theme theme = settings->get().themeData.theme;

	startTime = 0;
	lv_img_set_src(icon, THEMED_FILE(Icons, LockClosed, theme));

	if(config.start > config.end){
		lv_img_t* img = (lv_img_t*) icon;
		lv_obj_set_pos(icon, config.start - img->w, config.y);
	}else{
		lv_obj_set_pos(icon, config.start, config.y);
	}

	activity();
}

void Slider::activity(){
	activityTime = millis();
	lv_obj_clear_flag(icon, LV_OBJ_FLAG_HIDDEN);
}

void Slider::hide(){
	activityTime = 0;

	if(config.neverHide){
		return;
	}

	lv_obj_add_flag(icon, LV_OBJ_FLAG_HIDDEN);
}
