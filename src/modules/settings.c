#include "scrollMenu.h"
#include "core.h"
#include "icons.c"
#include "settings.h"
#include "date_adjust.h"

static struct menu_item menu_items[13] = { // first element reserved for text
    {"setting 1",  2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x06fe, clockDigital}}},
    {"setting 2",  2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"SL",         2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0xffff, trainIcon,  }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"uwu",        2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"wow",        2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, distorted,  }}},
    {"test",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"this works", 2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, distorted,  }}},
    {"yay",        2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}}
};

static struct scrollMenu menu = {
    .top = 20,
    .bottom = 238,
    .length = 13,
    .items = menu_items,
    .item_size = 73,
    .icon_top = 0,
    .icon_height = 49,
    .icon_width = 7
};


struct process settings = {
    .runExists = 1,
    .run = &settings_run,
    .startExists = 1,
    .start = &settings_init,
    .stopExists = 1,
    .stop = &settings_stop,
    .event = &event_always,
};

void settings_init() {    
    drawSquare(0, 0, 239, 319, 0x0000);
    scrollMenu_init(&menu);
    core_start_process(&statusbar);
}

void settings_run() {
    int selectedItem = drawScrollMenu(menu);

    if (selectedItem != -1) {
        display_scroll(320, 0, 0, 0);

        if (selectedItem == 2) {
            core_stop_process(&settings);
            core_start_process(&sl);
            return;
        }
        if (selectedItem == 0) {
            core_stop_process(&settings);
            core_start_process(&date_adjust);
            return;
        }
        if (selectedItem == 1) {
            sleep();
            drawSquare(0, 0, 239, 319, 0x0000);
            scrollMenu_init(&menu);
            display_backlight(255);
        }
    }
}

void settings_stop() {
    core_stop_process(&statusbar);
}
