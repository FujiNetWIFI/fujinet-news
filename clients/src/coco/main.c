/**
 * @brief   News Reader
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details.
 * @verbose Main Program
 */

#include <cmoc.h>
#include <coco.h>
#include "globals.h"
#include "topics.h"
#include "articles.h"
#include "article.h"
#include "cocotext.h"
#include "colorpicker.h"
#include "appkey.h"

void init(void)
{
    initCoCoSupport();
    init_color_palette();
    textMode = getTextMode();
    settings_load();
}

int main(void)
{
    clear_screen(1);
    init();

    while(1)
    {
        switch(state)
        {
        case TOPICS:
            state=topics();
            break;
        case ARTICLES:
            state=articles();
            break;
        case ARTICLE:
            state=article();
            break;
        case BYE:
            break;
        }

        if (state==BYE)
            break;
    }

    coldStart();

    return 0;
}
