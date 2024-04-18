#include "StatusBar.h"
#include "Util/Services.h"
#include "Filepaths.hpp"
#include "Settings/Settings.h"

StatusBar::StatusBar(lv_obj_t* parent, bool showExtra) : LVObject(parent), showExtra(showExtra){
	buildUI();

	// Events::listen(Facility::Phone, &queue); TODO: uncomment once evnet processing is actually hapening
}

void StatusBar::loop(){
	if(clock){
		clock->loop();
	}

	phone->loop();

	batDevice->loop();
}

void StatusBar::set24hFormat(bool format){
	if(!clock) return;

	clock->set24hFormat(format);
}

void StatusBar::updateVisuals() noexcept{
	if(phone != nullptr){
		phone->updateVisuals();
	}

	if(clock != nullptr){
		clock->updateVisuals();
	}

	if(batDevice != nullptr){
		batDevice->updateVisuals();
	}
}

void StatusBar::buildUI(){
	lv_obj_set_size(*this, 128, 15);
	lv_obj_set_style_pad_ver(*this, 2, 0);
	lv_obj_set_style_pad_hor(*this, 4, 0);

	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	left = lv_obj_create(*this);
	lv_obj_set_size(left, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_style_pad_gap(left, 2, 0);
	lv_obj_set_flex_flow(left, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(left, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

	phone = new PhoneElement(*this, showExtra);

	if(showExtra){
		clock = new ClockLabelSmall(*this);
		lv_obj_add_flag(*clock, LV_OBJ_FLAG_FLOATING);
		lv_obj_center(*clock);
	}

	batDevice = new MenuBatteryElement(left);
}
