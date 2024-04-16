#include "PickerElement.h"
#include "Theme/theme.h"
#include "LV_Interface/InputLVGL.h"
#include "Settings/Settings.h"
#include <Util/Services.h>

PickerElement::PickerElement(lv_obj_t* parent, const std::string& name, uint16_t startingIndex, const std::string& choiceNames, const std::function<void(uint16_t)>& cb) : LVSelectable(parent), startingIndex(startingIndex), cb(cb), choiceNames(choiceNames) {
	lv_obj_add_flag(*this, LV_OBJ_FLAG_CLICKABLE);

	buildStyles();
	buildUI(name);
}

void PickerElement::updateVisuals(){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_style_set_border_color(focusedStyle, settings->get().themeData.primaryColor);
	lv_style_set_text_color(labelStyle, settings->get().themeData.highlightColor);

	lv_obj_add_style(*this, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
	lv_obj_add_style(label, labelStyle, 0);
	lv_obj_add_style(picker, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(picker, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
	lv_obj_add_style(picker, labelStyle, LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_DEFAULT);
}

void PickerElement::buildStyles(){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_style_set_border_width(defaultStyle, 1);
	lv_style_set_border_opa(defaultStyle, 0);
	lv_style_set_pad_all(defaultStyle, 3);
	lv_style_set_bg_opa(defaultStyle, 0);

	lv_style_set_border_width(focusedStyle, 1);
	lv_style_set_border_color(focusedStyle, settings->get().themeData.primaryColor);
	lv_style_set_border_opa(focusedStyle, LV_OPA_COVER);

	lv_style_set_text_font(labelStyle, &devin);
	lv_style_set_text_color(labelStyle, settings->get().themeData.highlightColor);
	lv_style_set_text_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_opa(labelStyle, LV_OPA_COVER);
	lv_style_set_text_align(labelStyle, LV_TEXT_ALIGN_CENTER);
	lv_style_set_pad_all(labelStyle, 1);
}

void PickerElement::buildUI(const std::string& name){
	lv_obj_add_style(*this, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(*this, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);

	label = lv_label_create(*this);
	lv_obj_align(label, LV_ALIGN_LEFT_MID, 0, 0);
	lv_obj_add_style(label, labelStyle, 0);
	lv_label_set_text(label, name.c_str());

	picker = lv_roller_create(*this);
	lv_roller_set_options(picker, choiceNames.c_str(), LV_ROLLER_MODE_INFINITE);
	lv_roller_set_visible_row_count(picker, 1);
	lv_roller_set_selected(picker, startingIndex, LV_ANIM_OFF);
	lv_obj_align(picker, LV_ALIGN_RIGHT_MID, 0, 0);

	lv_obj_add_style(picker, defaultStyle, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_add_style(picker, focusedStyle, LV_PART_MAIN | LV_STATE_FOCUSED);
	lv_obj_add_style(picker, labelStyle, LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_DEFAULT);
	lv_obj_set_style_text_align(picker, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_FOCUSED | LV_STATE_DEFAULT);
	lv_obj_set_style_pad_hor(picker, 1, LV_PART_SELECTED);
	lv_obj_set_style_radius(picker, 3, LV_PART_MAIN | LV_STATE_DEFAULT);
	lv_obj_set_style_anim_time(picker, 250, LV_PART_MAIN);
	lv_obj_set_style_text_line_space(picker, 5, LV_PART_MAIN);
	lv_group_add_obj(inputGroup, picker);

	lv_obj_set_height(*this, 25);
	lv_obj_set_width(*this, lv_pct(100));

	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		auto element = (PickerElement*) e->user_data;
		element->deselect();
	}, LV_EVENT_CLICKED, this);

	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		if(lv_event_get_key(e) != LV_KEY_ENTER) return;

		lv_anim_del(e->target, nullptr);
	}, LV_EVENT_KEY, this);

	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		PickerElement* element = (PickerElement*) e->user_data;

		element->startAnim(e->target);
	}, LV_EVENT_FOCUSED, this);

	lv_obj_add_event_cb(picker, [](lv_event_t* e){
		PickerElement* element = (PickerElement*) e->user_data;
		if(element->cb){
			element->cb(lv_roller_get_selected(element->picker));
		}
	}, LV_EVENT_VALUE_CHANGED, this);

	lv_obj_add_event_cb(obj, [](lv_event_t* e){
		auto element = (PickerElement*) e->user_data;
		lv_group_set_editing(element->inputGroup, true);
	}, LV_EVENT_CLICKED, this);
}

void PickerElement::animFunc(void* var, int32_t val){
	lv_obj_set_style_border_opa((lv_obj_t*) var, val, LV_STATE_EDITED);
	lv_obj_invalidate((lv_obj_t*) var);
}

void PickerElement::startAnim(lv_obj_t* target){
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

uint8_t PickerElement::getValue() const{
	return lv_roller_get_selected(picker);
}
