#include "display.h"
#include "touch.h"
#include "nrf_delay.h"
#include <stdbool.h>
#include "wdt.h"

static bool Wake = 0;

void WakeInterrupt() {
    Wake = 1;
}

void system_sleep() {
    Wake = 0;
    display_backlight(0);
    subscribeTouchInterrupt(WakeInterrupt);

    touch_sleep();

    while (!Wake) {
        __WFI();
        wdt_feed();
    }

    touch_wake();
}
