#include "display.h"
#include "time_event_handler.h"
#include "touch.h"
#include "nrf_delay.h"
#include <stdbool.h>
#include "wdt.h"

void power_manager_init();
void power_manager_run();
void power_manager_stop();

static dependency dependencies[] = {{&display, running}, {&time_event_handler, running}};
static task tasks[] = {{&power_manager_init, start, 2, dependencies}, {&power_manager_run, run, 0}, {&power_manager_stop, stop, 0}};

process power_manager = {
    .taskCnt = 3,
    .tasks = tasks,
};

static bool Wake = 0;

void WakeInterrupt() {
    Wake = 1;
}

void system_sleep() {
    Wake = 0;
    subscribeTouchInterrupt(WakeInterrupt);

    touch_sleep();

    while (!Wake) {
        __WFI();
        wdt_feed();
    }

    touch_wake();
}

static int timeout= 150;

void set_system_timeout(int timeoutInTenthsOfSeconds) {
    timeout = timeoutInTenthsOfSeconds;
}

void power_manager_init() {
    power_manager.trigger = create_time_event(10);
}

void power_manager_stop() {
    free_time_event(power_manager.trigger);
}

void power_manager_run() {
    struct touchPoints touchPoint;
    touch_refresh(&touchPoint);

    static int counter = 0;
    if (touchPoint.event) {
        counter = 0;
    }

    if (counter > timeoutInTenthsOfSeconds) {
        display_backlight(0);
        system_sleep();
        display_backlight(255);
        counter = 0;
    }
    counter++;
}
