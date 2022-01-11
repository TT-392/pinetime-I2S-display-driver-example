#include "nrf_gpio.h"
#include "display.h"

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

int main(void) {
    nrf_gpio_cfg_output(15);	
    nrf_gpio_pin_write(15,1);
    nrf_gpio_cfg_input(13, NRF_GPIO_PIN_PULLDOWN);

    display_init();
    display_backlight(255);

    int width = 100;
    int height = 100;
    uint8_t dummy[width*height*2];

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            uint16_t color = convertColor((y * 0xff) / height,((height-y) * 0xff) / height,(x * 0xff) / width);
            dummy[(y*width + x)*2] = color >> 8;
            dummy[(y*width + x)*2+ 1] = color & 0xff;
        }
    }

    drawSquare_I2S(0, 0, 239, 239, 0x0000);
    //drawSquare_I2S(0, 0, 40, 0, 0xffff);
    drawBitmap_I2S(0, 0, 99, 1, dummy);
    while(1);
    while(1){
        drawSquare_I2S(0, 0, 239, 239, 0x0000);
        drawSquare_I2S(0, 0, 239, 239, 0xf800);
        drawSquare_I2S(0, 0, 239, 239, 0x001f);
        drawSquare_I2S(0, 0, 239, 239, 0x07e0);
    }
}
