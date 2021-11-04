#include "nrf_gpio.h"
#include "display.h"
#include "battery.h"
#include "main_menu.h"
#include "clock_pine.h"
#include "system.h"
#include "info.h"
#include "power_manager.h"
#include "flash.h"
#include "nrf_delay.h"
#include "frame.c"

int main(void) {
    clock_setup();
    battery_init();
    spiflash_init();

    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);

    system_task(start, &display);
    display_backlight(255);

    drawSquare(0, 0, 239, 239, 0x0000);
    drawSquare_I2S(0, 0, 239, 239, 0xf800);
    drawMono(0, 0, 239, 239, frame, 0x0000, 0xffff);
    drawMono_I2S(0, 0, 239, 239, frame, 0x0000, 0xffff);
    while(1);
    /*
    while(1);
    nrf_delay_ms(1000);
    drawSquare_I2S(0, 0, 219, 319, 0x0000);
    nrf_delay_ms(1000);
    drawSquare_I2S(0, 0, 219, 319, 0xffff);
    nrf_delay_ms(1000);
    drawSquare_I2S(0, 0, 219, 319, 0x0000);
    nrf_delay_ms(1000);
    drawSquare_I2S(0, 0, 219, 319, 0xffff);
    nrf_delay_ms(1000);
    drawSquare_I2S(0, 0, 219, 319, 0x0000);
    nrf_delay_ms(1000);*/

    system_task(start, &main_menu);
    system_task(start, &power_manager);
    display_backlight(255);

    while (1) {
        system_run();
    }
}
