#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"

#include "wdt.h"
#include "display.h"
#include "display_defines.h"
#include "display_print.h"
#include "battery.h"
#include "frame.c"
//#include "breakout.h"
//#include "modules/date.c"
//#include "modules/heart.c"
#include "semihost.h"
#include "statusbar.h"
//#include "scrollMenu.h"
#include "systick.h"
#include "watchface.h"
#include "date.h"
#include "touch.h"
#include "uart.h"
#include "main_menu.h"
#include <math.h>
#include "date_adjust.h"
#include "clock_pine.h"
#include "breakout.h"
#include "system.h"
#include "power_manager.h"
#include "steamLocomotive.h"

static bool toggle = 1;

int main(void) {
    clock_setup();
    battery_init();
    //date_init();
    //sysTick_init();
    //date_init();

    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);

    system_task(start, &display);
    drawSquare(0, 0, 239, 319, 0x0000);

    system_task(start, &main_menu);
    system_task(start, &power_manager);
    display_backlight(255);

    while (1) {
        system_run();
    }
}
