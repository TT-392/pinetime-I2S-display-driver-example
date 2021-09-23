#include <stdio.h>
#include "display.h"
#include "statusbar.h"
#include "battery_pine.h"
#include "display_print.h"
#include "settings.h"

void info_init();
void info_stop();
void info_run();

static dependency dependencies[] = {{&display, running}, {&statusbar, running}};
static task tasks[] = {{&info_init, start, 2, dependencies}, {&info_run, run, 0}, {&info_stop, stop, 0}};

process info = {
    .taskCnt = 3,
    .tasks = tasks,
    .trigger = &event_always
};

void info_init() {
    char buffer[30];
    int flags;
    float voltage, percent;

    battery_read(&flags, &voltage, &percent);

    snprintf(buffer, 30, "Compile time: %s", __DATE__);
    drawString(0, 30, buffer, 0xffff, 0x0000);
    drawString(0, 50, __TIME__, 0xffff, 0x0000);

    snprintf(buffer, "Battery voltage: %i", (int)3.2);
    drawString(0, 70, buffer, 0xffff, 0x0000);
    snprintf(buffer, "Battery voltage: %i", (int)percent);
    drawString(0, 90, buffer, 0xffff, 0x0000);

    NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
        2 << GPIOTE_CONFIG_POLARITY_Pos |
        13 << GPIOTE_CONFIG_PSEL_Pos;
}

void info_stop() {
    drawSquare(0, 20, 239, 239, 0x0000);
}

void info_run() {
    if (NRF_GPIOTE->EVENTS_IN[3]) {
        NRF_GPIOTE->EVENTS_IN[3] = 0;
        system_task(stop, &info);
        system_task(start, &settings);
    }

}

