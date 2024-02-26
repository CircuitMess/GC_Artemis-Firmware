#ifndef CLOCKSTAR_FIRMWARE_MENUITEM_H
#define CLOCKSTAR_FIRMWARE_MENUITEM_H

#include "LV_Interface/LVObject.h"
#include "LV_Interface/LVGIF.h"

class MenuItem : public LVObject {
public:
	MenuItem(lv_obj_t* parent, const char* imagePath, const char* labelText);

protected:
	lv_obj_t* imageContainer;
	lv_obj_t* image;
	lv_obj_t* label;

	void constructVis(const char* imagePath, const char* labelText);

	virtual void onFocus(){};

	virtual void onDefocus(){};

};


#endif //CLOCKSTAR_FIRMWARE_MENUITEM_H
