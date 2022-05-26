#include <nrf.h>
#include "display.h"
#include "I2S.h"
#include "nrf_delay.h"
#include "frame.c"
#include "nrf_gpio.h"
#include "display_defines.h"

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

int main() {
    int width = 120, height = 120;
    uint8_t data[width*height*2];


    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int g = ((255*x)/width) ^ ((255*y)/height);
            int b = ((255*(width-x))/width) ^ ((255*y)/height);

            uint16_t color = convertColor(0, g, b);
            data[(x + y*width) * 2] = color >> 8;
            data[(x + y*width) * 2 + 1] = color & 0xff;
        }
    }
    display_init();

    bool toggle = 0;
    int scroll = 0;
    uint8_t brightness = 0;
    while(1) {
        nrf_delay_ms(1);
        display_draw_rect(0, 0, 239, 239, 0xf800);
        nrf_delay_ms(1);
        display_draw_mono(0, 0, 239, 239, frame, 0x07e0, 0x001f);
        nrf_delay_ms(1);
        display_draw_bitmap(120, 0, 120 + width - 1, height - 1, data);
        display_draw_bitmap(0, 0, width - 1, height - 1, data);
        display_draw_bitmap(0, 120, width - 1, 120+height - 1, data);
        display_draw_bitmap(120, 120, 120+width - 1, 120+height - 1, data);
        invert(toggle);
        toggle = !toggle;
        scroll += 10;
        display_scroll(10, 220, 90, scroll % 220);

        brightness += 10;
        display_set_backlight(brightness);
    }
}
