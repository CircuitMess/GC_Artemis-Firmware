#ifndef CLOCKSTAR_FIRMWARE_SLIDERELEMENT_H
#define CLOCKSTAR_FIRMWARE_SLIDERELEMENT_H


#include <functional>
#include "LV_Interface/LVSelectable.h"
#include "LV_Interface/LVStyle.h"

class SliderElement : public LVSelectable {
public:
	explicit SliderElement(lv_obj_t* parent, const char* name, std::function<void(uint8_t)> cb, uint8_t value = 0);

	void setValue(uint8_t value); //0-100
	[[nodiscard]] uint8_t getValue() const; //0-100

	virtual void updateVisuals() override;

private:
	lv_obj_t* label;
	lv_obj_t* slider;

	static constexpr lv_style_selector_t sel = LV_PART_MAIN | LV_STATE_DEFAULT;
	static constexpr lv_style_selector_t selFocus = LV_PART_MAIN | LV_STATE_FOCUSED;
	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;
	LVStyle sliderMainStyle;
	LVStyle sliderKnobStyle;

	uint8_t value; //0-100
	std::function<void(uint8_t)> cb;

	static constexpr uint8_t SliderWidth = 55;
	static constexpr uint8_t SliderHeight = 12;
	static constexpr uint8_t SliderRange = 50;
	static constexpr uint8_t MaxValue = 100;
	static constexpr uint8_t Height = 23;

};


#endif //CLOCKSTAR_FIRMWARE_SLIDERELEMENT_H
