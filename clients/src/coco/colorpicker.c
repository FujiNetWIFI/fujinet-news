#include <cmoc.h>
#include <coco.h>
#include "globals.h"
#include "cocotext.h"
#include "bar.h"
#include "colorpicker.h"

byte g_normal_fg  = FG_BLACK;
byte g_normal_bg  = BG_GREEN;
byte g_hilight_fg = FG_WHITE;
byte g_hilight_bg = BG_BLUE;
byte g_attr_xor   = 0x1A;

#define BG_BLACK 1
#define BG_WHITE 7

#define NUM_SCHEMES   ((byte)5)
#define NUM_COLORSETS ((byte)4)

static byte g_scheme = 0;

/* n_fg, n_bg, h_fg, h_bg per scheme */
static const byte scheme_data[] = {
    FG_BLACK, BG_GREEN, FG_WHITE, BG_BLUE,
    FG_GREEN, BG_BLACK, FG_BLACK, BG_GREEN,
    FG_BLACK, BG_GREEN, FG_GREEN, BG_BLACK,
    FG_WHITE, BG_BLACK, FG_BLACK, BG_WHITE,
    FG_BLACK, BG_WHITE, FG_WHITE, BG_BLACK,
};

/* GIME 6-bit RGBrgb border codes indexed by BG palette value
 * (BG_GREEN=0, BG_BLACK=1, BG_BLUE=2, BG_WHITE=7) */
static const byte bg_rgb[8] = { 0x12, 0x00, 0x09, 0, 0, 0, 0, 0x3F };

static byte palette_backup[16];
static byte border_backup;

void init_color_palette(void)
{
    byte *pal = (byte *)0xFFB0;
    for (byte i = 0; i < 16; i++)
        palette_backup[i] = pal[i];
    border_backup = *((byte *)0xFF9A);

    *((byte *)0xFFB0) = 0x12;   /* green */
    *((byte *)0xFFB1) = 0x00;   /* black */
    *((byte *)0xFFB2) = 0x09;   /* blue  */
    *((byte *)0xFFB7) = 0x3F;   /* white */
}

void restore_color_palette(void)
{
    byte *pal = (byte *)0xFFB0;
    for (byte i = 0; i < 16; i++)
        pal[i] = palette_backup[i];
    *((byte *)0xFF9A) = border_backup;
}

/* Set the GIME border ($FF9A) to match the current normal background.
 * Palette registers are write-only, so use the known RGB code, not a read. */
void set_border(void)
{
    *((byte *)0xFF9A) = bg_rgb[g_normal_bg];
}

void pen_normal(void)
{
    attr(g_normal_fg, g_normal_bg, FALSE, FALSE);
}

static void apply_scheme(byte idx)
{
    const byte *s = scheme_data + (idx << 2);
    g_normal_fg  = s[0];
    g_normal_bg  = s[1];
    g_hilight_fg = s[2];
    g_hilight_bg = s[3];
    g_attr_xor   = ((g_hilight_fg << 3) | g_hilight_bg) ^
                   ((g_normal_fg  << 3) | g_normal_bg);
    pen_normal();
    set_border();
}

byte get_color_scheme(void)
{
    return g_scheme;
}

void set_color_scheme(byte idx)
{
    if (idx < NUM_SCHEMES)
    {
        g_scheme = idx;
        apply_scheme(g_scheme);
    }
}

byte get_hires_colorset(void)
{
    return colorset;
}

void set_hires_colorset(byte cs)
{
    if (cs < NUM_COLORSETS)
        colorset = cs;
}

/* Advance to the next color combination. Hires text (42-col) cycles the
 * PMODE4 colorset, which the GIME applies automatically. 40/80-col cycles
 * the attribute scheme; the caller redraws so the new colors take effect. */
void cycle_color_scheme(void)
{
    if (hirestxt_mode)
    {
        colorset = (colorset + 1) % NUM_COLORSETS;
        screen(1, (colorset >> 1) & 1);
        setScreenInverted(colorset & 1);
    }
    else
    {
        g_scheme = (g_scheme + 1) % NUM_SCHEMES;
        apply_scheme(g_scheme);
    }
}
