#include "TimePickerModal.h"
#include "Services/Time.h"
#include "Theme/theme.h"
#include "LV_Interface/InputLVGL.h"
#include <Util/Services.h>
#include <valarray>

TimePickerModal::TimePickerModal(LVScreen* parent, tm time) : LVModal(parent), time(time){
	buildStyles();
	buildUI();
	setDateLimits();
}

void TimePickerModal::buildStyles(){
	lv_style_set_border_width(defaultStyle, 1);
	lv_style_set_border_opa(defaultStyle, 0);
	lv_style_set_pad_all(defaultStyle, 1);
	lv_style_set_bg_opa(defaultStyle, 0);

	lv_style_set_border_width(focusedStyle, 1);
	lv_style_set_border_color(focusedStyle, lv_color_white());
	lv_style_set_border_opa(focusedStyle, LV_OPA_COVER);

	lv_style_set_text_font(labelStyle, &devin);
	lv_style_set_text_color(labelStyle, lv_color_white());
	lv_style_set_text_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_text_align(labelStyle, LV_TEXT_ALIGN_CENTER);
}

void TimePickerModal::buildUI(){
	lv_obj_set_layout(*this, LV_LAYOUT_FLEX);
	lv_obj_set_flex_align(*this, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(*this, LV_FLEX_FLOW_COLUMN);

	timeCont = lv_obj_create(*this);
	lv_obj_set_layout(timeCont, LV_LAYOUT_FLEX);
	lv_obj_set_flex_align(timeCont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(timeCont, LV_FLEX_FLOW_ROW);

	dateCont = lv_obj_create(*this);
	lv_obj_set_layout(dateCont, LV_LAYOUT_FLEX);
	lv_obj_set_flex_align(dateCont, LV_FLEX_ALIGN_SPACE_EVENLY, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
	lv_obj_set_flex_flow(dateCont, LV_FLEX_FLOW_ROW);
	lv_obj_set_style_border_color(timeCont, lv_palette_main(LV_PALETTE_RED), 0);

	hour = createPicker(timeCont, time.tm_hour, 0, 23);
	addLabel(timeCont, ":");
	minute = createPicker(timeCont, time.tm_min, 0, 59);
	addLabel(timeCont, ":");
	second = createPicker(timeCont, time.tm_sec, 0, 59);

	day = createPicker(dateCont, time.tm_mday, 1, 31);
	addLabel(dateCont, "/");

	month = lv_roller_create(dateCont);
	lv_roller_set_options(month, MonthsNames, LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(month, 1);
	lv_roller_set_selected(month, time.tm_mon, LV_ANIM_OFF);

	lv_obj_add_style(month, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(month, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
	lv_obj_add_style(month, labelStyle, LV_PART_MAIN);
	lv_obj_set_style_pad_hor(month, 1, LV_PART_SELECTED);
	lv_obj_set_style_bg_color(month, lv_palette_main(LV_PALETTE_INDIGO), LV_PART_SELECTED | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(month, LV_OPA_COVER, LV_PART_SELECTED | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_color(month, lv_palette_main(LV_PALETTE_LIGHT_BLUE), LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_bg_opa(month, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_radius(month, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_anim_time(month, 250, LV_PART_MAIN);
	lv_obj_set_style_text_line_space(month, 2, LV_PART_MAIN);
	lv_group_add_obj(inputGroup, month);

	lv_obj_add_event_cb(month, [](lv_event_t* e){
		if(lv_event_get_key(e) != LV_KEY_ENTER) return;

		lv_anim_del(e->target, nullptr);
	}, LV_EVENT_KEY, this);

	lv_obj_add_event_cb(month, [](lv_event_t* e){
		auto modal = (TimePickerModal*) e->user_data;

		if(lv_group_get_editing(modal->inputGroup)){
			modal->startAnim(e->target);
		}
	}, LV_EVENT_FOCUSED, this);


	year = createPicker(*this, time.tm_year + 1900, 1900, 9999);

	lv_obj_add_event_cb(month, [](lv_event_t* e){
		auto modal = (TimePickerModal*) e->user_data;
		modal->setDateLimits();
	}, LV_EVENT_VALUE_CHANGED, this);

	lv_obj_add_event_cb(year, [](lv_event_t* e){
		auto modal = (TimePickerModal*) e->user_data;
		modal->setDateLimits();
	}, LV_EVENT_VALUE_CHANGED, this);

	lv_obj_set_size(timeCont, lv_pct(100), LV_SIZE_CONTENT);
	lv_obj_set_size(dateCont, lv_pct(100), LV_SIZE_CONTENT);


	saveButton = new LabelElement(*this, "Save", [this](){
		saveTime();
		delete this;
	});
	lv_group_add_obj(inputGroup, *saveButton);
}

void TimePickerModal::saveTime(){
	auto& ts = *(Time*) Services.get(Service::Time);

	time.tm_hour = lv_spinbox_get_value(hour);
	time.tm_min = lv_spinbox_get_value(minute);
	time.tm_sec = lv_spinbox_get_value(second);
	time.tm_mday = lv_spinbox_get_value(day);
	time.tm_mon = lv_roller_get_selected(month);
	time.tm_year = lv_spinbox_get_value(year) - 1900;

	ts.setTime(time);
}

lv_obj_t* TimePickerModal::createPicker(lv_obj_t* parent, int32_t value, int32_t rangeMin, int32_t rangeMax){
	const auto maxDigits = (uint8_t) (std::log10(rangeMax) + 1);

	lv_obj_t* picker = lv_spinbox_create(parent);
	lv_spinbox_set_value(picker, value);
	lv_spinbox_set_range(picker, rangeMin, rangeMax);
	lv_spinbox_set_rollover(picker, true);
	lv_spinbox_set_digit_format(picker, maxDigits, 0);
	lv_obj_add_style(picker, focusedStyle, SelFocus);
	lv_obj_add_style(picker, defaultStyle, SelDefault);
	lv_obj_add_style(picker, labelStyle, LV_PART_MAIN);
	auto sb = (lv_spinbox_t*) picker;
	lv_obj_add_style(sb->ta.label, labelStyle, 0);

	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		if(lv_event_get_key(e) != LV_KEY_ENTER) return;

		auto modal = (TimePickerModal*) e->user_data;
		lv_group_set_editing(modal->inputGroup, false);
		InputLVGL::getInstance()->invertDirections(false);
		lv_anim_del(e->target, nullptr);
	}, LV_EVENT_KEY, this);


	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		lv_spinbox_set_cursor_pos(e->target, 0);
		auto modal = (TimePickerModal*) e->user_data;

		if(lv_group_get_editing(modal->inputGroup)){
			InputLVGL::getInstance()->invertDirections(true);
			modal->startAnim(e->target);
		}
	}, LV_EVENT_FOCUSED, this);

	lv_group_add_obj(inputGroup, picker);
	const auto width = (lv_coord_t) (lv_font_get_glyph_width(lv_obj_get_style_text_font(picker, LV_PART_MAIN), '0', '0') * (maxDigits + 1) + 2);
	lv_obj_set_width(picker, width);

	return picker;
}

void TimePickerModal::addLabel(lv_obj_t* parent, const char* text){
	lv_obj_t* label = lv_label_create(parent);
	lv_label_set_text(label, text);
	lv_obj_add_style(label, labelStyle, 0);
}

void TimePickerModal::setDateLimits(){
	int m = lv_roller_get_selected(month);
	int y = lv_spinbox_get_value(year);
	int daysLimit;

	static constexpr int daysInMonth[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

	if(m == 1 && ((y % 4 == 0 && y % 100 != 0) || (y % 400 == 0))){
		daysLimit = 29;
	}else{
		daysLimit = daysInMonth[m];
	}

	lv_spinbox_set_range(day, 1, daysLimit);
}

void TimePickerModal::animFunc(void* var, int32_t val){
	lv_obj_set_style_border_opa((lv_obj_t*) var, val, LV_STATE_EDITED);
	lv_obj_invalidate((lv_obj_t*) var);
}

void TimePickerModal::startAnim(lv_obj_t* target){
	lv_anim_init(&blinkAnim);
	lv_anim_set_exec_cb(&blinkAnim, animFunc);
	lv_anim_set_values(&blinkAnim, LV_OPA_0, LV_OPA_100);
	lv_anim_set_time(&blinkAnim, 350);
	lv_anim_set_playback_time(&blinkAnim, 350);
	lv_anim_set_path_cb(&blinkAnim, lv_anim_path_step);
	lv_anim_set_repeat_count(&blinkAnim, LV_ANIM_REPEAT_INFINITE);
	lv_anim_set_var(&blinkAnim, target);
	lv_anim_start(&blinkAnim);
}
