#include "ShutdownScreen.h"
#include "Theme/theme.h"
#include "Util/stdafx.h"
#include "Filepaths.hpp"

ShutdownScreen::ShutdownScreen(){
	bg = lv_obj_create(*this);
	lv_obj_add_flag(bg, LV_OBJ_FLAG_FLOATING);
	lv_obj_set_size(bg, 128, 128);
	lv_obj_set_pos(bg, 0, 0);
	lv_obj_set_style_bg_color(bg, lv_color_black(), 0);
	lv_obj_set_style_bg_opa(bg, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_img_src(bg, File::Background, 0);


	lv_obj_set_size(*this, 128, 128);
	lv_obj_set_style_pad_bottom(*this, 16, 0);

	label = lv_label_create(*this);
	lv_obj_align(label, LV_ALIGN_BOTTOM_MID, 0, 0);
	lv_label_set_text(label, "Low battery!\nShutting down.");
	lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_font(label, &devin, 0);
	lv_obj_set_style_text_color(label, lv_color_white(), 0);

	img = lv_img_create(*this);
	lv_img_set_src(img, File::Menu::Theme1::BigLowBattery);
	lv_obj_center(img);
}

void ShutdownScreen::onStart(){
	blinkTime = millis();
	shutdownTime = millis();
}

void ShutdownScreen::loop(){
	if(millis() - blinkTime >= BlinkPeriod){
		if(lv_obj_has_flag(img, LV_OBJ_FLAG_HIDDEN)){
			lv_obj_clear_flag(img, LV_OBJ_FLAG_HIDDEN);
		}else{
			lv_obj_add_flag(img, LV_OBJ_FLAG_HIDDEN);
		}
		blinkTime = millis();
	}

	if(millis() - shutdownTime >= ShutdownTime){
		shutdown();
	}
}

void ShutdownScreen::shutdown(){
	//TODO - add shutdown sequence
}
