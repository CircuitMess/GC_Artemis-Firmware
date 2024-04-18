#ifndef CLOCKSTAR_FIRMWARE_ITEM_H
#define CLOCKSTAR_FIRMWARE_ITEM_H


#include <functional>
#include "LV_Interface/LVStyle.h"
#include "LV_Interface/LVSelectable.h"
#include "Notifs/Notif.h"
#include "CtrlItem.h"

class Item : public LVSelectable {
public:
	Item(lv_obj_t* parent, std::function<void()> dismiss);

	void update(const Notif& notif);
	const char* iconPath();

private:
	LVStyle standard;
	LVStyle focused;

	void initStyle();

	const char* iPath = nullptr;

	lv_obj_t* top;
	lv_obj_t* icon;
	lv_obj_t* label;

	lv_obj_t* bot;
	lv_obj_t* body;

	lv_obj_t* ctrl = nullptr;
	CtrlItem* del = nullptr;
	CtrlItem* canc = nullptr;

	void createControls();
	void delControls();

	const std::function<void()> onDismiss;

	static constexpr uint8_t LabelHeight = 8;
};


#endif //CLOCKSTAR_FIRMWARE_ITEM_H
