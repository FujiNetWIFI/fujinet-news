/**
 * @brief   News Reader
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details.
 * @verbose Bar routines
 */

#ifndef BAR_H
#define BAR_H

#include <cmoc.h>
#include <coco.h>

#define SCREEN_RAM_TOP_32 0x0400

void bar(byte y);
void fill_attr(unsigned int addr, unsigned int cnt, byte val);
void set_screen_attrs(byte val);
void shadow(int y, int c);

#endif /* BAR_H */
