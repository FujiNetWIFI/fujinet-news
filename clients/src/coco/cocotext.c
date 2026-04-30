#include <cmoc.h>
#include <coco.h>
#include "fujinet-fuji.h"
#include "hirestxt.h"
#include "globals.h"
#include "bar.h"
#include "cocotext.h"

//(byte *)*(byte *)0x00BC << 8 This is where BASIC puts it by default
#define SCREEN_BUFFER (byte*) 0xA00

/**
 * @brief Temp space for strupr(s) output, so original strings doesn't get changed.
 */
static char uppercase_tmp[321];

byte colorset = 0;
bool hirestxt_mode = false;

void hirestxt_init(void)
{
    hirestxt_mode = true;

    // Define a `HiResTextScreenInit` object:
    struct HiResTextScreenInit init =
        {
            42,                           /* characters per row */
            writeCharAt_42cols,           /* must be consistent with previous field */
            SCREEN_BUFFER,                /* pointer to the text screen buffer */
            TRUE,                         /* redirects printf() to the 42x24 text screen */
            (word *)0x112,                /* pointer to a 60 Hz async counter (Color Basic's TIMER) */
            0,                            /* default cursor blinking rate */
            NULL,                         /* use inkey(), i.e., Color Basic's INKEY$ */
            NULL,                         /* no sound on '\a' */
        };

    width(32);                               /* PMODE graphics will only appear from 32x16 (does nothing on CoCo 1&2) */
    pmode(4, (byte *)init.textScreenBuffer); /* hires text mode */
    pcls(255);
    screen(1, colorset);
    initHiResTextScreen(&init);
}

void hirestxt_close(void)
{
    if (hirestxt_mode)
    {
        hirestxt_mode = false;
        closeHiResTextScreen();
        width(32);
        pmode(0, 0);
        screen(0, 0);
        clear_screen(255);
    }
}

// Cycles colorset through 0..3. Bit 1 selects the screen color set
// (passed to screen()); bit 0 selects inverted video (setScreenInverted()).
void switch_colorset(void)
{
    if (hirestxt_mode)
    {
        colorset = (colorset + 1) & 3;

        screen(1, (colorset >> 1) & 1);
        setScreenInverted(colorset & 1);
    }
}

void gotoxy(byte x, byte y)
{
    if (hirestxt_mode)
    {
        moveCursor(x, y);
    }
    else
    {
        locate(x, y);
    }
}

void cursor(unsigned char onoff)
{
    if (onoff)
    {
        animateCursor();
    }
    else
    {
        removeCursor();
    }
}

void clear_screen(byte color)
{
    if (hirestxt_mode)
    {
        clrscr();
    }
    else
    {
        cls(color);
    }
}

void reverse(bool onoff)
{
    if (textMode != 32) // Nothing to do for 32
    {
        if (hirestxt_mode)
        {
            setInverseVideoMode(onoff);
        }
        else // CoCo3 40/80 mode (
        {
            if (onoff)
            {
                attr(FG_WHITE, BG_BLUE, FALSE, FALSE);
            }
            else
            {
                attr(FG_BLACK, BG_GREEN, FALSE, FALSE);
            }
        }
    }
}

void print_reverse(byte x, byte y, const char *text, bool lowercase_only)
{
    byte *sp;
    int o = 0;
    int len = strlen(text);

    gotoxy(x, y);

    if (textMode == 32)
    {
        putstr(text, len);
        if (!lowercase_only) // Reverse everything
        {
            sp = (byte *)SCREEN_RAM_TOP_32 + (int)((int)y * 32) + x;

            for (int i = 0; i < len; i++)
            {
                // Only change non-semigraphics characters
                if (*sp < 0x80)
                    *sp ^= 0x40;
                sp++;
            }
        }
    }
    else // Hires txt or CoCo3 40/80 mode
    {
        if (lowercase_only)
        {
            for (int i = 0; i < len; i++)
            {
                if (text[i] >= 'a' && text[i] <= 'z')
                {
                    reverse(1);
                    putchar(text[i] - 32);
                }
                else
                {
                    reverse(0);
                    putchar(text[i]);
                }
            }
            reverse(0);
        }
        else
        {
            reverse(1);
            putstr(text, len);
            reverse(0);
        }
    }
}

/**
 * @brief Return uppercase string without modifying original
 */
char *screen_upper(const char *s)
{
    memset(uppercase_tmp, 0, sizeof(uppercase_tmp));
    strcpy(uppercase_tmp, s);

    return strupr(uppercase_tmp);
}

/**
 * @brief Return uppercase string without modifying original
 */
char *screen_lower(const char *s)
{
    memset(uppercase_tmp, 0, sizeof(uppercase_tmp));
    strcpy(uppercase_tmp, s);

    return strlwr(uppercase_tmp);
}

void set_text_width(byte screen_width)
{
    // Do nothing if we're not really changing the width
    if (screen_width == textMode)
    {
        return;
    }

    if (screen_width == 42)
    {
        // If we're not already in hires text mode, we need to initialize it.
        hirestxt_init();
    }
    else
    {
        if (hirestxt_mode)
        {
            hirestxt_close();
        }

        width(screen_width);
    }

    textMode = screen_width;
}

void width_highlight(int i, byte y, byte width, bool highlight)
{
    char buf[10];
    sprintf(buf, "%d", (int)width);

    if (highlight)
    {
        print_reverse((byte)i * 3, y, buf, false);
    }
    else
    {
        gotoxy((byte)i * 3, y);
        printf(buf);
    }
}

byte text_width_menu(void)
{
    unsigned char menu_line;
    byte max_choice;
    int choice = 0;
    unsigned char widths[4];
    if (isCoCo3)
    {
        widths[0] = 32;
        widths[1] = 40;
        widths[2] = 42;
        widths[3] = 80;
        max_choice = 3;
    }
    else
    {
        widths[0] = 32;
        widths[1] = 42;
        max_choice = 1;
    }

    menu_line = (textMode == 32) ? 15 : 23;

    print_reverse(0, menu_line -1, "l/r TO SELECT, break TO CANCEL", true);
    gotoxy(0, menu_line);
    printf("                               ");
    gotoxy(0, menu_line);
    for (byte i = 0; i < max_choice + 1; i++)
    {
        if (widths[i] == textMode)
        {
            choice = i;
            width_highlight(i, menu_line, widths[i], true);
        }
        else
        {
            width_highlight(i, menu_line, widths[i], false);
        }
    }

    while (true)
    {
        // Move the cursor to the end of the last line
        gotoxy(textMode - 1, menu_line);
        switch (waitkey(false))
        {
        case ARROW_LEFT:
            width_highlight(choice, menu_line, widths[choice], false);
            choice--;
            if (choice < 0)
            {
                choice = 0;
            }
            width_highlight(choice, menu_line, widths[choice], true);
            break;
        case ARROW_RIGHT:
            width_highlight(choice, menu_line, widths[choice], false);
            choice++;
            if (choice > max_choice)
            {
                choice = max_choice;
            }
            width_highlight(choice, menu_line, widths[choice], true);
            break;
        case ENTER:
            return widths[choice];
            break;
        case BREAK:
            return (unsigned char)WIDTH_CANCEL;
            break;
        }
    }
}

void show_fetching_msg(bool clear, bool articles)
{
    if (textMode == 32)
    {
        if (clear)
        {
            if (articles)
            {
                clear_screen(2);
            }
            else
            {
                clear_screen(3);
            }
        }
        gotoxy(0, 0);
        printf("%s", fetching_buf);
    }
    else
    {
        if (clear)
        {
            clear_screen(1);
        }
        hd_bar(0, fetching_buf, false);
    }
}

byte cgetc_cursor()
{
    byte shift = false;
    byte k;

    while (true)
    {
        k = waitKeyBlinkingCursor();

        if (isKeyPressed(KEY_PROBE_SHIFT, KEY_BIT_SHIFT))
        {
            shift = 0x00;
        }
        else
        {
            if (k > '@' && k < '[')
                shift = 0x20;
        }

        if (k)
            return k + shift;
    }
}

void get_line(char *buf, uint8_t max_len)
{
    uint8_t c;
    uint16_t i = 0;
    uint8_t init_x = getCursorColumn();

    if (hirestxt_mode)
    {

        do
        {
            gotox((byte)i + init_x);
            cursor(1);
            c = cgetc_cursor();

            if (isprint(c))
            {
                putchar(c);
                buf[i] = c;
                if (i < max_len - 1)
                    i++;
            }
            else if (c == ARROW_LEFT)
            {
                if (i)
                {
                    putchar(ARROW_LEFT);
                    putchar(' ');
                    putchar(ARROW_LEFT);
                    --i;
                }
            }
        } while (c != ENTER);
        putchar('\n');
        buf[i] = '\0';

        cursor(0);
    }
    else
    {
        strcpy(buf, readline());
    }
}
