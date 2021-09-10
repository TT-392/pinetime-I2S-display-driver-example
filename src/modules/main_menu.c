#include "scrollMenu.h"
#include "display.h"
#include "main_menu.h"
#include "settings.h"
#include "steamLocomotive.h"
#include "breakout.h"
#include "system.h"
#include "touch.h"
#include "icons.h"
#include "statusbar.h"
#include "watchface.h"

void menu_init();
void menu_run();
void menu_stop();

static dependency dependencies[] = {{&display, running}, {&touch, running}, {&statusbar, running}};
static task tasks[] = {{&menu_init, start, 3, dependencies}, {&menu_run, run, 0}, {&menu_stop, stop}};

process main_menu = {
    .taskCnt = 3,
    .tasks = tasks,
    .trigger = &event_always
};

static struct menu_item menu_items[13] = { // first element reserved for text
    {"clock",      2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x06fe, clockDigital}}},
    {"SL",         2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0xffff, trainIcon,  }}},
    {"settings",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x528A, settings_circled,}}},
    {"breakout",       2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0xffff, icon_breakout,     }}},
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
    .length = 12,
    .items = menu_items,
    .item_size = 73,
    .icon_top = 0,
    .icon_height = 49,
    .icon_width = 7
};


void menu_init() {    
    scrollMenu_init(&menu);
    //core_start_process(&statusbar);
}

void menu_run() {
    int selectedItem = drawScrollMenu(menu);

    if (selectedItem != -1) {

        if (selectedItem == 0) {
            system_task(stop, &main_menu);
            system_task(start, &watchface);
        }
        if (selectedItem == 1) {
            system_task(stop, &main_menu);
            system_task(start, &sl);
        }
        if (selectedItem == 2) {
            system_task(stop, &main_menu);
            system_task(start, &settings);
        }
        if (selectedItem == 3) {
            system_task(stop, &main_menu);
            system_task(start, &breakout);
        }
        if (selectedItem == 4) {
        }
    }
}

void menu_stop() {
    drawSquare(0, 20, 239, 239, 0x0000);
    display_scroll(320, 0, 0, 0);
}
