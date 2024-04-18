#include "LabelElement.h"

#include <utility>
#include "Theme/theme.h"
#include "Settings/Settings.h"
#include "Util/Services.h"

LabelElement::LabelElement(lv_obj_t* parent, const char* name, std::function<void()> cb, bool reverseColorScheme, lv_align_t textAlign) : LVObject(parent), cb(std::move(cb)), reverseColor(reverseColorScheme){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_obj_add_flag(*this, LV_OBJ_FLAG_CLICKABLE);

	lv_style_set_border_width(defaultStyle, 1);
	lv_style_set_border_opa(defaultStyle, 0);
	lv_style_set_pad_all(defaultStyle, 3);
	lv_style_set_bg_opa(defaultStyle, 0);

	lv_style_set_border_width(focusedStyle, 1);
	lv_style_set_border_color(focusedStyle, reverseColor ? settings->get().themeData.secondaryColor : settings->get().themeData.primaryColor);
	lv_style_set_border_opa(focusedStyle, LV_OPA_COVER);

	lv_style_set_text_font(labelStyle, &devin);
	lv_style_set_text_color(labelStyle, reverseColor ? settings->get().themeData.backgroundColor : settings->get().themeData.highlightColor);

	if(reverseColor){
		lv_obj_set_style_bg_color(*this, settings->get().themeData.highlightColor, 0);
		lv_obj_set_style_bg_opa(*this, LV_OPA_COVER, 0);
	}else{
		lv_obj_set_style_bg_opa(*this, 0, 0);
	}

	lv_obj_set_height(*this, Height);
	lv_obj_set_width(*this, lv_pct(100));

	lv_obj_add_style(*this, focusedStyle, SelFocus);
	lv_obj_add_style(*this, defaultStyle, SelDefault);

	label = lv_label_create(*this);
	lv_obj_align(label, textAlign, 0, 0);
	lv_obj_add_style(label, labelStyle, 0);
	lv_label_set_text(label, name);

	lv_obj_add_event_cb(obj, [](lv_event_t* e){
		auto element = static_cast<LabelElement*>(e->user_data);
		if(element->cb) element->cb();
	}, LV_EVENT_CLICKED, this);

}

void LabelElement::updateVisuals(){
	Settings* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_style_set_text_color(labelStyle, reverseColor ? settings->get().themeData.backgroundColor : settings->get().themeData.highlightColor);
	lv_obj_set_style_bg_color(*this, reverseColor ? settings->get().themeData.highlightColor : settings->get().themeData.backgroundColor, 0);

	lv_style_set_border_color(focusedStyle, settings->get().themeData.primaryColor);
	lv_style_set_text_color(labelStyle, settings->get().themeData.highlightColor);

	lv_obj_add_style(*this, focusedStyle, SelFocus);
	lv_obj_add_style(label, labelStyle, 0);
}
