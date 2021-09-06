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
//#include "statusbar.h"
//#include "scrollMenu.h"
#include "systick.h"
//#include "watchface.h"
#include "date.h"
#include "touch.h"
#include "uart.h"
//#include "main_menu.h"
#include "audio.h"
#include "bad_apple_midi.h"
#include <math.h>
//#include "date_adjust.h"
#include "clock_pine.h"
#include "breakout.h"
#include "system.h"

static bool toggle = 1;

int main(void) {
    //clock_setup();
    //battery_init();
    //date_init();
    //sysTick_init();
    //date_init();
    bool osRunning = 1;


    int x = 0;


    int i = 320;
    int counter = 0;


    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);


    bool backlight = 0;

    //display_init();
    //drawMono(0, 0, 239, 319, frame, 0x0000, 0xffff);

    //touch_init();
    system_task(start, &breakout);

    display_backlight(255);

    //breakout_init();
    while (1) {
     //   breakout_run();
        system_run();
    }

   // core_start_process(&main_menu);
   // core_start_process(&statusbar);

   // while(osRunning) {
   //     core_run();
   //     wdt_feed();
   // }
}
