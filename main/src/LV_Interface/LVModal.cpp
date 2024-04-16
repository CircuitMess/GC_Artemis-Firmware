#include "LVModal.h"
#include "LVScreen.h"
#include "InputLVGL.h"
#include "Filepaths.hpp"
#include "Settings/Settings.h"
#include "Util/Services.h"

LVModal* LVModal::current = nullptr;

LVModal::LVModal(LVScreen* parent) : LVObject((lv_obj_t*) *parent), parentScreen(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	delete current;
	current = this;

	inputGroup = lv_group_create();

	container = lv_obj_create(*parent);
	lv_obj_set_parent(*this, container);

	lv_obj_add_event_cb(obj, [](lv_event_t* event){
		auto container = (lv_obj_t*) event->user_data;
		lv_obj_del_async(container);
	}, LV_EVENT_DELETE, container);

	lv_obj_set_size(container, 110, 92);
	lv_obj_set_style_pad_all(container, 6, 0);
	lv_obj_set_style_bg_color(container, settings->get().themeData.backgroundColor, 0);
	lv_obj_set_style_bg_opa(container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_color(container, settings->get().themeData.primaryColor, 0);
	lv_obj_set_style_border_opa(container, LV_OPA_COVER, 0);
	lv_obj_set_style_border_width(container, 1, 0);

	lv_obj_set_size(*this, 86, 76);
	lv_obj_set_align(*this, LV_ALIGN_CENTER);

	lv_obj_set_align(container, LV_ALIGN_CENTER);
	lv_obj_add_flag(container, LV_OBJ_FLAG_FLOATING);

	lv_indev_set_group(InputLVGL::getInstance()->getIndev(), inputGroup);
}

LVModal::~LVModal(){
	lv_indev_set_group(InputLVGL::getInstance()->getIndev(), parentScreen->getInputGroup());
	lv_group_del(inputGroup);
	current = nullptr;
}
