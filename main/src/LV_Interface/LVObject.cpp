#include "LVObject.h"

LVObject::LVObject(lv_obj_t* parent){
	obj = lv_obj_create(parent);

	lv_obj_clear_flag(obj, LV_OBJ_FLAG_SCROLLABLE);

	lv_obj_add_event_cb(obj, [](lv_event_t* event){
		auto obj = static_cast<LVObject*>(event->user_data);
		if(!obj->deleting){
			obj->deleting = true;
			delete obj;
		}
	}, LV_EVENT_DELETE, this);
}

LVObject::~LVObject(){
	if(!deleting){
		deleting = true;
		lv_obj_del((lv_obj_t*) obj);
	}
}

LVObject::operator lv_obj_t*(){
	return obj;
}

void LVObject::updateVisuals(){

}

