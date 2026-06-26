#include <cmoc.h>
#include <coco.h>
#include "appkey.h"
#include "globals.h"
#include "cocotext.h"
#include "colorpicker.h"

#define SETTINGS_LEN 3

static uint8_t io_buffer[MAX_APPKEY_LEN];

bool read_appkey(uint8_t *buffer, uint8_t max_len, uint8_t *out_len)
{
    uint16_t read_count = 0;

    fuji_set_appkey_details(APPKEY_CREATOR_ID, APPKEY_APP_ID, DEFAULT);

    if (fuji_read_appkey(COCO_NEWS_APP_KEY, &read_count, io_buffer) && read_count > 0)
    {
        if (read_count > max_len)
            read_count = max_len;
        memcpy(buffer, io_buffer, read_count);
        *out_len = (uint8_t)read_count;
        return true;
    }

    *out_len = 0;
    return false;
}

bool write_appkey(const uint8_t *buffer, uint8_t len)
{
    fuji_set_appkey_details(APPKEY_CREATOR_ID, APPKEY_APP_ID, DEFAULT);
    return fuji_write_appkey(COCO_NEWS_APP_KEY, len, (uint8_t *)buffer);
}

static bool is_valid_width(byte w)
{
    if (w == 32 || w == 42)
        return true;
    if (isCoCo3 && (w == 40 || w == 80))
        return true;
    return false;
}

void settings_load(void)
{
    uint8_t buf[SETTINGS_LEN];
    uint8_t len = 0;
    byte width = 0;

    if (read_appkey(buf, SETTINGS_LEN, &len))
    {
        if (len >= 2)
            set_color_scheme(buf[1]);
        if (len >= 3)
            set_hires_colorset(buf[2]);
        if (len >= 1 && is_valid_width(buf[0]))
            width = buf[0];
    }

    if (width == 0)
        width = (textMode == 32) ? 42 : textMode;

    set_text_width(width);
}

void settings_save(void)
{
    uint8_t buf[SETTINGS_LEN];

    buf[0] = textMode;
    buf[1] = get_color_scheme();
    buf[2] = get_hires_colorset();

    write_appkey(buf, SETTINGS_LEN);
}
