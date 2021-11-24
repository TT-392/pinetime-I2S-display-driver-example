#include "date.h"
#include "battery.h"
#include "nrf_gpio.h"
#include "clock_pine.h"
#include "display.h"
#include "system.h"
#include "power_manager.h"
#include "display_print.h"
#include <stdlib.h>

void statusbar_run();
void statusbar_init() {statusbar_run();}
void statusbar_stop();

static dependency dependencies[] = {{&display, running}};
static task tasks[] = {{&statusbar_init, start, 1, dependencies}, {&statusbar_run, run, 0}, {&statusbar_stop, stop, 0}};

process statusbar = {
    .taskCnt = 3,
    .tasks = tasks,
    .trigger = &secondPassed
};

void statusbar_run() {
    uint16_t color = 0xffff; 
    if (!nrf_gpio_pin_read(19))
        color = 0x67EC;
    battery_draw_percent(200,0,color,0x0000);
    drawDate(0,0,"%H:%M:%S");

    char buffer[30];
    sprintf(buffer, "%i %i %i  ", power_manager_get_current_count(), power_manager_get_current_timeout(), power_manager_get_current_dummyCount() % 1000);
    drawString(100, 0, buffer, 0xffe0, 0x0000);
}

void statusbar_stop() {
    drawSquare(0, 0, 239, 19, 0x0000);
}
