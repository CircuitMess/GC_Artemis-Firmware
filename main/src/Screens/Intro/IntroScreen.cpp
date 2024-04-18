#include "IntroScreen.h"
#include "Util/stdafx.h"
#include "Screens/Lock/LockScreen.h"
#include "LV_Interface/FSLVGL.h"
#include "Filepaths.hpp"
#include "Services/StatusCenter.h"
#include "Util/Services.h"

IntroScreen::IntroScreen(){
	bg = lv_obj_create(*this);
	lv_obj_set_style_bg_img_opa(bg, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(bg, File::Intro::OrangeBackground, 0);
	lv_obj_move_background(bg);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);

	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	for(int i = 0; i < 3; ++i){
		imageConts[i] = lv_obj_create(*this);
		lv_obj_set_size(imageConts[i], 128, 128);
		images[i] = lv_img_create(imageConts[i]);
		lv_img_set_src(images[i], ImagePaths[i]);
		lv_obj_center(images[i]);
	}

	for(const auto img : Cached){
		FSLVGL::addToCache(img);
	}
}

IntroScreen::~IntroScreen(){
	for(const auto img : Cached){
		FSLVGL::removeFromCache(img);
	}
}

void IntroScreen::onStart(){
	if(StatusCenter* status = (StatusCenter*) Services.get(Service::Status)){
		status->circularBlink();
	}

	animTime = millis();
	lv_anim_init(&anim);
	lv_anim_set_exec_cb(&anim, animFuncScroll);
	lv_anim_set_var(&anim, *this);
	lv_anim_set_time(&anim, ScrollTime);
	lv_anim_set_path_cb(&anim, lv_anim_path_ease_in_out);
}

void IntroScreen::onStop(){
	if(StatusCenter* status = (StatusCenter*) Services.get(Service::Status)){
		status->circularBlink();
	}

	lv_anim_del(*this, nullptr);
}

void IntroScreen::loop(){
	if(state == Scrolling && millis() - animTime >= ScrollTime){
		state = Waiting;
		animTime = millis();
	}else if(state == Waiting && millis() - animTime >= WaitTime){
		currentIndex++;
		if(currentIndex >= 3){
			transition([](){ return std::make_unique<LockScreen>(); });
			return;
		}else if(currentIndex >= 2){
			gotoPhase2();
			lv_anim_set_var(&anim, phase2);
			lv_anim_set_values(&anim, 0, LV_OPA_COVER);
			lv_anim_set_exec_cb(&anim, animFuncOpa);
		}else{
			const auto currentScroll =  lv_obj_get_scroll_y(*this);
			lv_anim_set_values(&anim, currentScroll, currentScroll +  128);
		}
		state = Scrolling;
		animTime = millis();
		lv_anim_start(&anim);
	}
}

void IntroScreen::gotoPhase2(){
	phase2 = lv_obj_create(*this);
	lv_obj_add_flag(phase2, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(phase2, 128, 128);
	lv_obj_set_pos(phase2, 0, 0);

	lv_obj_set_style_bg_img_src(phase2, File::Intro::BlackBackground, 0);
	lv_obj_set_style_bg_img_opa(phase2, LV_OPA_COVER, 0);

	auto img = lv_img_create(phase2);
	lv_img_set_src(img, File::Intro::Artemis);
	lv_obj_center(img);

	lv_obj_set_style_opa(phase2, 0, 0);
}

void IntroScreen::animFuncScroll(void* obj, int32_t val){
	lv_obj_scroll_to_y((lv_obj_t*)obj, val, LV_ANIM_OFF);
}

void IntroScreen::animFuncOpa(void* obj, int32_t val){
	lv_obj_set_style_opa((lv_obj_t*) obj, val, 0);
}
