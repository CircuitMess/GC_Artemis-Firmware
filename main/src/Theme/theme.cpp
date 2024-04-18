#include <cstdio>
#include "theme.h"

static lv_theme_t theme;

static struct {
	lv_style_t scr;
	lv_style_t label;
	lv_style_t roller;
} styles;

static void theme_apply(lv_theme_t* th, lv_obj_t* obj){
	if(lv_obj_get_parent(obj) == nullptr){
		lv_obj_add_style(obj, &styles.scr, 0);
	}else if(lv_obj_check_type(obj, &lv_label_class)) {
		lv_obj_add_style(obj, &styles.label, 0);
	}else if(lv_obj_check_type(obj, &lv_roller_class)){
		lv_obj_add_style(obj, &styles.roller, 0);
	}
}

lv_theme_t* theme_init(lv_disp_t* disp){
	lv_style_init(&styles.scr);
	lv_style_set_bg_color(&styles.scr, lv_color_black());
	lv_style_set_bg_opa(&styles.scr, LV_OPA_COVER);

	lv_style_init(&styles.label);
	lv_style_set_text_font(&styles.label, &devin);
	lv_style_set_text_color(&styles.label, lv_color_make(207, 198, 184));
	lv_style_set_text_line_space(&styles.label, 2);

	lv_style_init(&styles.roller);
	lv_style_set_bg_color(&styles.roller, lv_color_black());
	lv_style_set_bg_opa(&styles.roller, LV_OPA_TRANSP);

	memset(&theme, 0, sizeof(lv_theme_t));
	theme.apply_cb = theme_apply;
	theme.parent = lv_disp_get_theme(disp);
	theme.disp = disp;
	theme.color_primary = lv_color_make(244, 126, 27);
	theme.color_secondary = lv_color_make(207, 198, 184);
	theme.font_small = &devin;
	theme.font_normal = &devin;
	theme.font_large = &clockfont;
	return &theme;
}
