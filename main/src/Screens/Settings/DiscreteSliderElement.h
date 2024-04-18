#ifndef CLOCKSTAR_FIRMWARE_DISCRETESLIDERELEMENT_H
#define CLOCKSTAR_FIRMWARE_DISCRETESLIDERELEMENT_H

#include <functional>
#include "LV_Interface/LVSelectable.h"
#include "LV_Interface/LVStyle.h"

class DiscreteSliderElement : public LVSelectable {
public:
	explicit DiscreteSliderElement(lv_obj_t* parent, const char* name, std::function<void(uint8_t)> cb, std::vector<const char*> displayValues, uint8_t value = 0);

	void setValue(uint8_t value); //0-100
	[[nodiscard]] uint8_t getValue() const; //0-100

	virtual void updateVisuals() override;

private:
	lv_obj_t* label;
	lv_obj_t* slider;
	lv_obj_t* valueLabel;

	static constexpr lv_style_selector_t sel = LV_PART_MAIN | LV_STATE_DEFAULT;
	static constexpr lv_style_selector_t selFocus = LV_PART_MAIN | LV_STATE_FOCUSED;
	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;
	LVStyle sliderMainStyle;
	LVStyle sliderKnobStyle;

	uint8_t value; // 0 - (displayValues.size() - 1)
	std::function<void(uint8_t)> cb;
	std::vector<const char*> displayValues;

	static constexpr uint8_t SliderWidth = 55;
	static constexpr uint8_t SliderHeight = 12;
	static constexpr uint8_t SliderRange = 50;
	static constexpr uint8_t Height = 23;
};


#endif //CLOCKSTAR_FIRMWARE_DISCRETESLIDERELEMENT_H
