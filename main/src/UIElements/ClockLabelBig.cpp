#include "ClockLabelBig.h"
#include "LV_Interface/FSLVGL.h"
#include "Settings/Settings.h"
#include "Util/Services.h"

ClockLabelBig::ClockLabelBig(lv_obj_t* parent, bool vertical, int16_t verticalPad) : ClockLabel(parent), vertical(vertical){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return;
	}

	lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);

	if(vertical){
		lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
	}else{
		lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
	}

	lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

	if(settings->get().themeData.theme == Theme::Theme3 || settings->get().themeData.theme == Theme::Theme4){
		lv_obj_set_style_pad_gap(obj, 0, 0);
	}else{
		lv_obj_set_style_pad_gap(obj, 4, 0);
	}

	hours = lv_obj_create(obj);
	minutes = lv_obj_create(obj);

	lv_obj_set_style_pad_bottom(hours, verticalPad, 0);
	lv_obj_set_style_pad_top(minutes, verticalPad, 0);

	lv_obj_set_size(hours, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_size(minutes, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(hours, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_flow(minutes, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(hours, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
	lv_obj_set_flex_align(minutes, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);

	if(settings->get().themeData.theme == Theme::Theme4){
		lv_obj_set_style_pad_gap(hours, 0, 0);
		lv_obj_set_style_pad_gap(minutes, 0, 0);
	}else{
		lv_obj_set_style_pad_gap(hours, 4, 0);
		lv_obj_set_style_pad_gap(minutes, 4, 0);
	}

	hourIcons[0] = lv_img_create(hours);
	hourIcons[1] = lv_img_create(hours);
	minuteIcons[0] = lv_img_create(minutes);
	minuteIcons[1] = lv_img_create(minutes);

	if(!vertical){
		colonIcon = lv_img_create(hours);
		lv_obj_set_style_pad_hor(colonIcon, 1, 0);
	}

	updateTime(ts.getTime());
}

void ClockLabelBig::updateUI(const char* clockText, const char* ps){
	lv_img_set_src(hourIcons[0], getPath(clockText[0]));
	lv_img_set_src(hourIcons[1], getPath(clockText[1]));

	if(colonIcon != nullptr){
		lv_img_set_src(colonIcon, getPath(clockText[2]));
	}

	lv_img_set_src(minuteIcons[0], getPath(clockText[3]));
	lv_img_set_src(minuteIcons[1], getPath(clockText[4]));
}

const char* ClockLabelBig::getPath(char c){
	auto* settings = (Settings*) Services.get(Service::Settings);
	if(settings == nullptr){
		return "";
	}

	const Theme theme = settings->get().themeData.theme;

	const char* IconPaths[12] = {
			THEMED_FILE(Clock, Num0, theme),
			THEMED_FILE(Clock, Num1, theme),
			THEMED_FILE(Clock, Num2, theme),
			THEMED_FILE(Clock, Num3, theme),
			THEMED_FILE(Clock, Num4, theme),
			THEMED_FILE(Clock, Num5, theme),
			THEMED_FILE(Clock, Num6, theme),
			THEMED_FILE(Clock, Num7, theme),
			THEMED_FILE(Clock, Num8, theme),
			THEMED_FILE(Clock, Num9, theme),
			THEMED_FILE(Clock, Space, theme),
			THEMED_FILE(Clock, Colon, theme),
	};

	if(c >= '0' && c <= '9'){
		return IconPaths[c - '0'];
	}else if(c == ':'){
		return IconPaths[10];
	}else{
		return IconPaths[11];
	}
}

