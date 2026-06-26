#include <cmoc.h>
#include <coco.h>
#include "globals.h"
#include "cocotext.h"
#include "hirestxt.h"
#include "bar.h"
#include "colorpicker.h"

static unsigned char bar_saved_ffa6;
static unsigned int  bar_addr;
static unsigned char bar_len;
static unsigned char bar_fill_val;
static unsigned int  bar_attr_cnt;

void bar(byte y)
{
    byte *sp;

    if (textMode == 32)
    {
        sp = (byte *)SCREEN_RAM_TOP_32 + ((int)y << 5);
        for (int i = 0; i < 32; i++)
        {
            if (*sp < 0x80)
                *sp ^= 0x40;
            sp++;
        }
    }
    else if (hirestxt_mode)
    {
        for (byte c = 0; c < textMode; c++)
            writeCharAt_42cols(c, y, 0);
    }
    else
    {
        bar_addr = 0xC000 + (unsigned int)y * (unsigned int)textMode * 2 + 1;
        bar_len  = textMode;
        asm
        {
            orcc    #$50
            lda     $FFA6
            sta     _bar_saved_ffa6
            lda     #$36
            sta     $FFA6

            ldx     _bar_addr
            ldb     _bar_len
@bar_loop
            lda     ,x
            eora    _g_attr_xor
            sta     ,x
            leax    2,x
            decb
            bne     @bar_loop

            lda     _bar_saved_ffa6
            sta     $FFA6
            andcc   #$AF
        }
    }
}

/* Fill a run of attribute bytes on the 40/80-col text screen.
 * addr points at the first attribute byte (char byte + 1); cnt is the
 * number of cells; every other byte (the attribute) is set to val. */
void fill_attr(unsigned int addr, unsigned int cnt, byte val)
{
    bar_addr     = addr;
    bar_attr_cnt = cnt;
    bar_fill_val = val;
    asm
    {
        orcc    #$50
        lda     $FFA6
        sta     _bar_saved_ffa6
        lda     #$36
        sta     $FFA6

        ldx     _bar_addr
        ldy     _bar_attr_cnt
        lda     _bar_fill_val
@fill_loop
        sta     ,x
        leax    2,x
        leay    -1,y
        bne     @fill_loop

        lda     _bar_saved_ffa6
        sta     $FFA6
        andcc   #$AF
    }
}

/* Fill all attribute bytes on the 40/80-col text screen with val. */
void set_screen_attrs(byte val)
{
    fill_attr(0xC001, (unsigned int)textMode * 24, val);
}

void shadow(int y, int c)
{
    byte *sp = (byte *)SCREEN_RAM_TOP_32;
    int o = y << 5;

    sp += o;

    *sp = (byte)c | 0x0b;
    memset(sp + 1, c | 0x03, 31);
}
