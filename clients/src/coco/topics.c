/**
 * @brief   News Reader
 * @author  Thomas Cherryhomes
 * @email   thom dot cherryhomes at gmail dot com
 * @license gpl v. 3, see LICENSE for details.
 * @verbose Topics menu
 */

#include <cmoc.h>
#include <coco.h>
#include "globals.h"
#include "topics.h"
#include "bar.h"
#include "cocotext.h"
#include "colorpicker.h"
#include "appkey.h"

#define MENU_Y_16 3
#define MENU_Y_24 8

TopicState topicState = TOPICS_RESET;

enum _selected_topic
{
    TOP_STORIES = 0,
    WORLD_NEWS,
    BUSINESS,
    SCIENCE,
    TECHNOLOGY,
    HEALTH,
    ENTERTAINMENT,
    POLITICS,
    SPORTS
} selectedTopic = TOP_STORIES;

int topicChanged = 0; // flag for updating selection bar

/**
 * @brief Reset topics menu
 */
TopicState topics_reset(void)
{
    selectedTopic = TOP_STORIES;
    topicChanged = 1;
    return TOPICS_DISPLAY;
}

TopicState topics_display(void)
{
    if (textMode == 32)
    {
        clear_screen(3);
        gotoxy(0, 0);
        printf("      FUJINET  NEWS TOPICS      ");
        shadow(1, 0xA0);
        bar(0);

        gotoxy(0, MENU_Y_16);

        printf(" \x99 %s\n", screen_upper(topicStrings[0]));
        printf(" \x9F %s\n", screen_upper(topicStrings[1]));
        printf(" \x8F %s\n", screen_upper(topicStrings[2]));
        printf(" \xAF %s\n", screen_upper(topicStrings[3]));
        printf(" \xBF %s\n", screen_upper(topicStrings[4]));
        printf(" \xCF %s\n", screen_upper(topicStrings[5]));
        printf(" \xDF %s\n", screen_upper(topicStrings[6]));
        printf(" \xEF %s\n", screen_upper(topicStrings[7]));
        printf(" \xFF %s\n", screen_upper(topicStrings[8]));

        shadow(0x0C, 0xA0);

        gotoxy(0, 14);
        printf("w TO CHANGE SCREEN WIDTH");

        gotoxy(0, 15);
        printf("SELECT WITH ARROWS, THEN ENTER.");
    }
    else
    {
        clear_screen(1);

        gotoxy(0, 0);
        printf("%*s%s%*s", (int)((textMode - 20) >> 1), "", "FUJINET  NEWS TOPICS", (int)((textMode - 20) >> 1), "");
        bar(0);

        for (byte i = 0; i < 9; i++)
        {
            gotoxy(0, i + MENU_Y_24);
            printf("%-*s", (int)(textMode - 1), topicStrings[i]);
        }

        print_reverse(0, 22, "c COLORS   w TO CHANGE SCREEN WIDTH", true);

        gotoxy(0, 23);
        printf("%-*s", (int)(textMode - 1), "SELECT WITH ARROWS, THEN ENTER.");
    }

    return TOPICS_MENU;
}

void select_screen_width(void)
{
    byte width_return = text_width_menu();

    if (width_return != WIDTH_CANCEL)
    {
        set_text_width(width_return);
    }
}

TopicState topics_menu()
{
    if (topicChanged)
    {
        topicChanged = 0;
        bar((byte)selectedTopic + (textMode == 32 ? MENU_Y_16 : MENU_Y_24));
        gotoxy(textMode == 32 ? 31 : textMode - 1, textMode == 32 ? 15 : 23);
    }

    while (true)
    {
        switch (waitkey(false))
        {
        case ARROW_UP:
            return TOPICS_UP;
        case ARROW_DOWN:
            return TOPICS_DOWN;
        case ENTER:
            return TOPICS_SELECT;
        case BREAK:
            return TOPICS_EXIT;
        case 'C':
        case 'c':
            cycle_color_scheme();
            settings_save();
            topicChanged = 1;
            return TOPICS_DISPLAY;
        case 'W':
        case 'w':
            select_screen_width();
            settings_save();
            return TOPICS_RESET;
        }
    }
}

TopicState topics_up()
{
    if (selectedTopic > TOP_STORIES)
    {
        bar((byte)selectedTopic + (textMode == 32 ? MENU_Y_16 : MENU_Y_24));
        gotoxy(textMode == 32 ? 31 : textMode - 1, textMode == 32 ? 15 : 23);
        selectedTopic--;
        topicChanged = 1;
    }

    return TOPICS_MENU;
}

TopicState topics_down()
{
    if (selectedTopic < SPORTS)
    {
        bar((byte)selectedTopic + (textMode == 32 ? MENU_Y_16 : MENU_Y_24));
        gotoxy(textMode == 32 ? 31 : textMode - 1, textMode == 32 ? 15 : 23);
        selectedTopic++;
        topicChanged = 1;
    }

    return TOPICS_MENU;
}

State topics(void)
{
    topicState=TOPICS_RESET;

    while (state == TOPICS)
    {
        switch(topicState)
        {
        case TOPICS_RESET:
            topicState = topics_reset();
            break;
        case TOPICS_DISPLAY:
            topicState = topics_display();
            break;
        case TOPICS_MENU:
            topicState = topics_menu();
            break;
        case TOPICS_UP:
            topicState = topics_up();
            break;
        case TOPICS_DOWN:
            topicState = topics_down();
            break;
        case TOPICS_SELECT:
            return ARTICLES;
        case TOPICS_EXIT:
            return BYE;
        }
    }

    return BYE;
}
