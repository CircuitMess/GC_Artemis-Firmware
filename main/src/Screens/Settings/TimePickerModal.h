#ifndef ARTEMIS_FIRMWARE_TIMEPICKERMODAL_H
#define ARTEMIS_FIRMWARE_TIMEPICKERMODAL_H

#include <ctime>
#include "LV_Interface/LVModal.h"
#include "LabelElement.h"

class TimePickerModal : public LVModal {
public:
	TimePickerModal(LVScreen* parent, tm time);

private:
	void buildStyles();
	void buildUI();
	void saveTime();

	lv_obj_t* createPicker(lv_obj_t* parent, int32_t value, int32_t rangeMin, int32_t rangeMax);
	void addLabel(lv_obj_t* parent, const char* text);
	void setDateLimits();

	tm time;

	lv_obj_t* hour, * minute, * second, * day, * month, * year, * meridiem;
	lv_obj_t* timeCont;
	lv_obj_t* dateCont;
	LabelElement* saveButton;

	LVStyle defaultStyle;
	LVStyle focusedStyle;
	LVStyle labelStyle;

	const bool startingInputInversion;
	const bool timeFormat24h;

	lv_anim_t blinkAnim;
	static void animFunc(void* var, int32_t val);
	void startAnim(lv_obj_t* target);

	static constexpr lv_style_selector_t SelDefault = LV_PART_MAIN | LV_STATE_DEFAULT;
	static constexpr lv_style_selector_t SelFocus = LV_PART_MAIN | LV_STATE_FOCUSED;
	static constexpr const char* MonthsNames = "January\nFebruary\nMarch\nApril\nMay\nJune\nJuly\nAugust\nSeptember\nOctober\nNovember\nDecember";
	static constexpr const char* MeridiemNames = "AM\nPM";
};


#endif //ARTEMIS_FIRMWARE_TIMEPICKERMODAL_H
