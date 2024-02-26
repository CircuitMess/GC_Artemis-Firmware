#include "ClockLabelBig.h"
#include "LV_Interface/FSLVGL.h"

ClockLabelBig::ClockLabelBig(lv_obj_t* parent) : ClockLabel(parent){
	lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
	lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_ROW);
	lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_START);
	lv_obj_set_style_pad_gap(obj, 4, 0);

	for(auto& icon : icons){
		icon = lv_img_create(obj);
	}


	lv_obj_set_style_pad_hor(icons[2], 1, 0);

	updateTime(ts.getTime());
}

void ClockLabelBig::updateUI(const char* clockText){
	for(uint8_t i = 0; i < NumIcons; i++){
		lv_img_set_src(icons[i], getPath(clockText[i]));
	}
}

constexpr const char* ClockLabelBig::getPath(char c){
	if(c >= '0' && c <= '9'){
		return IconPaths[c - '0'];
	}else if(c == ':'){
		return IconPaths[10];
	}else{
		return IconPaths[11];
	}
}

