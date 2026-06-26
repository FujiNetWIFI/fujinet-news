#ifndef COLORPICKER_H
#define COLORPICKER_H

#include <cmoc.h>
#include <coco.h>

extern byte g_normal_fg;
extern byte g_normal_bg;
extern byte g_hilight_fg;
extern byte g_hilight_bg;
extern byte g_attr_xor;

void init_color_palette(void);
void restore_color_palette(void);
void set_border(void);
void pen_normal(void);
void cycle_color_scheme(void);
byte get_color_scheme(void);
void set_color_scheme(byte idx);
byte get_hires_colorset(void);
void set_hires_colorset(byte cs);

#endif /* COLORPICKER_H */
