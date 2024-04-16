#ifndef ARTEMIS_FIRMWARE_PICKERELEMENT_H
#define ARTEMIS_FIRMWARE_PICKERELEMENT_H

#include <functional>
#include <string>
#include "LV_Interface/LVSelectable.h"
#include "LV_Interface/LVStyle.h"

class PickerElement : public LVSelectable {
public:
	PickerElement(lv_obj_t* parent, const std::string& name, uint16_t startingIndex, const std::string& choiceNames, const std::function<void(uint16_t)>& cb);
	virtual void updateVisuals() override;

	uint8_t getValue() const;

private:
	void buildStyles();
	void buildUI(const std::string& name);
	static void animFunc(void* var, int32_t val);
	void startAnim(lv_obj_t* target);

	uint16_t startingIndex;
	std::function<void(uint16_t)> cb;
	std::string choiceNames;

	lv_obj_t* label;
	lv_obj_t* picker;

	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;

	lv_anim_t blinkAnim;
};

#endif //ARTEMIS_FIRMWARE_PICKERELEMENT_H