#include "ClockLabelSmall.h"
#include "Theme/theme.h"
#include "Settings/Settings.h"
#include "Util/Services.h"

ClockLabelSmall::ClockLabelSmall(lv_obj_t* parent) : ClockLabel(parent){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	clock = lv_label_create(obj);
	lv_obj_set_style_text_font(clock, &devin2, 0);
	lv_obj_set_style_text_color(clock, settings->get().themeData.highlightColor, 0);

	updateTime(ts.getTime());
}

void ClockLabelSmall::updateVisuals(){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_obj_set_style_text_color(clock, settings->get().themeData.highlightColor, 0);
}

void ClockLabelSmall::updateUI(const char* clockText, const char* ps){
	std::string str = clockText;
	if(ps){
		str += " ";
		str += ps;
	}
	lv_label_set_text(clock, str.c_str());
}
