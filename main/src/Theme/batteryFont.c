/*******************************************************************************
 * Size: 7 px
 * Bpp: 1
 * Opts: 
 ******************************************************************************/

#ifdef LV_LVGL_H_INCLUDE_SIMPLE
#include "lvgl.h"
#else
#include "lvgl/lvgl.h"
#endif

#ifndef BATTERYFONT
#define BATTERYFONT 1
#endif

#if BATTERYFONT

/*-----------------
 *    BITMAPS
 *----------------*/

/*Store the image of the glyphs*/
static LV_ATTRIBUTE_LARGE_CONST const uint8_t glyph_bitmap[] = {
    /* U+0030 "0" */
    0x7b, 0x3d, 0xf7, 0xed, 0xe0,

    /* U+0031 "1" */
    0x6e, 0x66, 0x6f,

    /* U+0032 "2" */
    0x76, 0xc6, 0xed, 0xfc,

    /* U+0033 "3" */
    0x76, 0xc6, 0x6d, 0xb8,

    /* U+0034 "4" */
    0x1c, 0xf6, 0xf3, 0xfc, 0x30,

    /* U+0035 "5" */
    0xfe, 0x3c, 0x3d, 0xb8,

    /* U+0036 "6" */
    0x76, 0xf1, 0xed, 0xb8,

    /* U+0037 "7" */
    0xf1, 0x36, 0x66,

    /* U+0038 "8" */
    0x76, 0xf6, 0xed, 0xb8,

    /* U+0039 "9" */
    0x76, 0xf6, 0xfd, 0xb8
};


/*---------------------
 *  GLYPH DESCRIPTION
 *--------------------*/

static const lv_font_fmt_txt_glyph_dsc_t glyph_dsc[] = {
    {.bitmap_index = 0, .adv_w = 0, .box_w = 0, .box_h = 0, .ofs_x = 0, .ofs_y = 0} /* id = 0 reserved */,
    {.bitmap_index = 0, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 5, .adv_w = 70, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 8, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 12, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 16, .adv_w = 112, .box_w = 6, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 21, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 25, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 29, .adv_w = 84, .box_w = 4, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 32, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2},
    {.bitmap_index = 36, .adv_w = 98, .box_w = 5, .box_h = 6, .ofs_x = 0, .ofs_y = 2}
};

/*---------------------
 *  CHARACTER MAPPING
 *--------------------*/



/*Collect the unicode lists and glyph_id offsets*/
static const lv_font_fmt_txt_cmap_t cmaps[] =
{
    {
        .range_start = 48, .range_length = 10, .glyph_id_start = 1,
        .unicode_list = NULL, .glyph_id_ofs_list = NULL, .list_length = 0, .type = LV_FONT_FMT_TXT_CMAP_FORMAT0_TINY
    }
};



/*--------------------
 *  ALL CUSTOM DATA
 *--------------------*/

#if LVGL_VERSION_MAJOR == 8
/*Store all the custom data of the font*/
static  lv_font_fmt_txt_glyph_cache_t cache;
#endif

#if LVGL_VERSION_MAJOR >= 8
static const lv_font_fmt_txt_dsc_t font_dsc = {
#else
static lv_font_fmt_txt_dsc_t font_dsc = {
#endif
    .glyph_bitmap = glyph_bitmap,
    .glyph_dsc = glyph_dsc,
    .cmaps = cmaps,
    .kern_dsc = NULL,
    .kern_scale = 0,
    .cmap_num = 1,
    .bpp = 1,
    .kern_classes = 0,
    .bitmap_format = 0,
#if LVGL_VERSION_MAJOR == 8
    .cache = &cache
#endif
};


/*-----------------
 *  PUBLIC FONT
 *----------------*/

/*Initialize a public general font descriptor*/
#if LVGL_VERSION_MAJOR >= 8
const lv_font_t batteryFont = {
#else
lv_font_t batteryFont = {
#endif
    .get_glyph_dsc = lv_font_get_glyph_dsc_fmt_txt,    /*Function pointer to get glyph's data*/
    .get_glyph_bitmap = lv_font_get_bitmap_fmt_txt,    /*Function pointer to get glyph's bitmap*/
    .line_height = 6,          /*The maximum line height required by the font*/
    .base_line = -2,             /*Baseline measured from the bottom of the line*/
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



#endif /*#if BATTERYFONT*/

