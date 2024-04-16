/*******************************************************************************
 * Size: 8 px
 * Bpp: 1
 * Opts:
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef LANDERFONT
#define LANDERFONT 1
#endif

#if LANDERFONT

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
/* U+0020 " " */
0x0,

/* U+002D "-" */
0xe0,

/* U+002E "." */
0x80,

/* U+002F "/" */
0x5a, 0x80,

/* U+0030 "0" */
0x74, 0x6b, 0x17, 0x0,

/* U+0031 "1" */
0x59, 0x2e,

/* U+0032 "2" */
0xf1, 0xf8, 0xf0,

/* U+0033 "3" */
0xe5, 0x1e,

/* U+0034 "4" */
0x35, 0x9f, 0x10,

/* U+0035 "5" */
0xf8, 0xe1, 0xe0,

/* U+0036 "6" */
0xf8, 0xf9, 0xf0,

/* U+0037 "7" */
0xf1, 0x24, 0x40,

/* U+0038 "8" */
0xf9, 0xf9, 0xf0,

/* U+0039 "9" */
0xf9, 0xf1, 0xf0,

/* U+003A ":" */
0xa0,

/* U+0041 "A" */
0xf9, 0x9f, 0x90,

/* U+0042 "B" */
0xf9, 0xe9, 0xf0,

/* U+0043 "C" */
0xf8, 0x88, 0xf0,

/* U+0044 "D" */
0xe9, 0x99, 0xe0,

/* U+0045 "E" */
0xf3, 0xce,

/* U+0046 "F" */
0xf3, 0xc8,

/* U+0047 "G" */
0xf8, 0xb9, 0xf0,

/* U+0048 "H" */
0x99, 0xf9, 0x90,

/* U+0049 "I" */
0xe9, 0x2e,

/* U+004A "J" */
0x72, 0x22, 0xc0,

/* U+004B "K" */
0x9a, 0xca, 0x90,

/* U+004C "L" */
0x92, 0x4e,

/* U+004D "M" */
0x8e, 0xeb, 0x18, 0x80,

/* U+004E "N" */
0x9d, 0xb9, 0x90,

/* U+004F "O" */
0xf9, 0x99, 0xf0,

/* U+0050 "P" */
0xf9, 0x9f, 0x80,

/* U+0051 "Q" */
0xf9, 0x9b, 0xf0,

/* U+0052 "R" */
0xf9, 0x9e, 0x90,

/* U+0053 "S" */
0xf8, 0xf1, 0xf0,

/* U+0054 "T" */
0xe9, 0x24,

/* U+0055 "U" */
0x99, 0x99, 0xf0,

/* U+0056 "V" */
0x8c, 0x62, 0xa2, 0x0,

/* U+0057 "W" */
0x8c, 0x6b, 0x5d, 0x80,

/* U+0058 "X" */
0xb5, 0x5a,

/* U+0059 "Y" */
0xb6, 0xa4,

/* U+005A "Z" */
0xf1, 0x68, 0xf0
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
		{.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
		{.bitmap_index = 0, .adv_w = 48, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 0},
		{.bitmap_index = 1, .adv_w = 64, .box_w = 3, .box_h = 1, .ofs_x = 0, .ofs_y = 6},
		{.bitmap_index = 2, .adv_w = 32, .box_w = 1, .box_h = 1, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 3, .adv_w = 48, .box_w = 2, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 5, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 9, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 11, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 14, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 16, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 19, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 22, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 25, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 28, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 31, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 34, .adv_w = 32, .box_w = 1, .box_h = 3, .ofs_x = 0, .ofs_y = 5},
		{.bitmap_index = 35, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 38, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 41, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 44, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 47, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 49, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 51, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 54, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 57, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 59, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 62, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 65, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 67, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 71, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 74, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 77, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 80, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 83, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 86, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 89, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 91, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 94, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 98, .adv_w = 96, .box_w = 5, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 102, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 104, .adv_w = 64, .box_w = 3, .box_h = 5, .ofs_x = 0, .ofs_y = 4},
		{.bitmap_index = 106, .adv_w = 80, .box_w = 4, .box_h = 5, .ofs_x = 0, .ofs_y = 4}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
		{
				{
						.range_start = 32, .range_length = 1, .glyph_id_start = 1,
						.unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
				},
				{
						.range_start = 45, .range_length = 14, .glyph_id_start = 2,
						.unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
				},
				{
						.range_start = 65, .range_length = 26, .glyph_id_start = 16,
						.unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
				}
		};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR >= 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
		.glyph_bitmap = glyph_bitmap,
		.glyph_dsc = glyph_dsc,
		.cmaps = cmaps,
		.kern_dsc = NULL,
		.kern_scale = 0,
		.cmap_num = 3,
		.bpp = 1,
		.kern_classes = 0,
		.bitmap_format = 0,
#if LVGL_VERSION_MAJOR >= 8
		.cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t landerfont = {
#else
lv_font_t landerfont = {
#endif
		.get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
		.get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
		.line_height = 9,          /*The maximum line height required by the font*/
		.base_line = 0,             /*Baseline measured from the bottom of the line*/
#if !(LVGL_VERSION_MAJOR == 6 && LVGL_VERSION_MINOR == 0)
		.subpx = LV_FONT_SUBPX_NONE,
#endif
#if LV_VERSION_CHECK(7, 4, 0) || LVGL_VERSION_MAJOR >= 8
		.underline_position = 1,
		.underline_thickness = 0,
#endif
		.dsc = &font_dsc,          /*The custom font data. Will be accessed by `get_glyph_bitmap/dsc` */
		.fallback = NULL,
		.user_data = NULL
};



#endif /*#if LANDERFONT*/
