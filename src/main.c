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
#include "audio.h"
#include "bad_apple_midi.h"
#include <math.h>
#include "date_adjust.h"
#include "clock_pine.h"
#include "breakout.h"
#include "system.h"
#include "steamLocomotive.h"
#include <stdio.h>

static bool toggle = 1;

int main(void) {
    system_task(start, &display);
    drawSquare(0, 0, 239, 319, 0x0000);
    display_backlight(255);

    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);

    audio_init();

    int pulseWidth;
    for (int i = 9; i < 100; i++) {
        pulseWidth = i*10;
        char buffer[30];
        sprintf(buffer, "%i", pulseWidth);
        drawString(0,0,buffer,0xffff,0x0000);
        audio_set_freq(20, pulseWidth);

        while (!nrf_gpio_pin_read(13));
        nrf_delay_ms(100);
        while (nrf_gpio_pin_read(13));
        nrf_delay_ms(100);
    }
    while(1);


    clock_setup();
    battery_init();
    //date_init();
    //sysTick_init();
    //date_init();



    system_task(start, &main_menu);
    display_backlight(255);

    while (1) {
        system_run();
    }
}
