#ifndef BATCONTROLLER_FIRMWARE_LVOBJECT_H
#define BATCONTROLLER_FIRMWARE_LVOBJECT_H

#include <lvgl.h>

class LVObject {
public:
	LVObject(lv_obj_t* parent);
	virtual ~LVObject();

	operator lv_obj_t*();

	virtual void updateVisuals();

protected:
	lv_obj_t* obj = nullptr;

private:
	bool deleting = false;
};


#endif //BATCONTROLLER_FIRMWARE_LVOBJECT_H
