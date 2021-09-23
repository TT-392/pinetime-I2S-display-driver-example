#include "scrollMenu.h"
#include "system.h"
#include "main_menu.h"
#include "icons.h"
#include "display.h"
#include "touch.h"
#include "info.h"
#include "statusbar.h"
#include "date_adjust.h"

void settings_init();
void settings_run();
void settings_stop();

static dependency dependencies[] = {{&display, running}, {&touch, running}, {&statusbar, running}};
static task tasks[] = {{&settings_init, start, 3, dependencies}, {&settings_run, run, 0}, {&settings_stop, stop, 0}};

process settings = {
    .taskCnt = 3,
    .tasks = tasks,
    .trigger = &event_always
};


static struct menu_item menu_items[13] = { // first element reserved for text
    {"adjust time",  2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x06fe, clockDigital}}},
    {"info",  2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
    {"back",  2, {{70, 28, 0, 0, 0xffff},{0, 12, 55, 60, 0x00f0, termux,     }}},
};

static struct scrollMenu menu = {
    .top = 20,
    .bottom = 238,
    .length = 3,
    .items = menu_items,
    .item_size = 73,
    .icon_top = 0,
    .icon_height = 49,
    .icon_width = 7
};

void settings_init() {    
    scrollMenu_init(&menu);
    //core_start_process(&statusbar);
}

void settings_run() {
    int selectedItem = drawScrollMenu(menu);

    if (selectedItem != -1) {
        display_scroll(320, 0, 0, 0);

        if (selectedItem == 0) {
            system_task(stop, &settings);
            system_task(start, &date_adjust);
            return;
        }
        if (selectedItem == 1) {
            system_task(stop, &settings);
            system_task(start, &info);
            return;
        }
        if (selectedItem == 2) {
            system_task(stop, &settings);
            system_task(start, &main_menu);
            return;
        }
    }
}

void settings_stop() {
    drawSquare(0, 19, 239, 239, 0x0000);
}
