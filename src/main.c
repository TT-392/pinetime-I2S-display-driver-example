#include "nrf_gpio.h"
#include "display.h"
#include "battery.h"
#include "main_menu.h"
#include "clock_pine.h"
#include "system.h"
#include "info.h"
#include "power_manager.h"

int main(void) {
    clock_setup();
    battery_init();

    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);

    system_task(start, &display);
    drawSquare(0, 0, 239, 319, 0x0000);
    display_backlight(255);
    system_task(start, &info);
    while (1) {
        system_run();
    }
    system_task(start, &display);
    drawSquare(0, 0, 239, 319, 0x0000);

    system_task(start, &main_menu);
    system_task(start, &power_manager);
    display_backlight(255);

    while (1) {
        system_run();
    }
}
