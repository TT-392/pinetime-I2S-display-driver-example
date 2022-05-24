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


    I2S_BITMAP_t bmp = {.bitmap = data};
    I2S_MONO_t mono = {.bitmap = frame,
    .color_fg = 0xffff,
    .color_bg = 0x0000};

    I2S_SOLID_COLOR_t color = {.color = 0xf800};

    while(1) {
        I2S_writeBlock(0, 0, 239, 239, &color, SOLID_COLOR);
        I2S_writeBlock(0, 0, 239, 239, &mono, MONO);
        I2S_writeBlock(120, 0, 120 + width - 1, height - 1, &bmp, BITMAP);
        I2S_writeBlock(0, 0, width - 1, height - 1, &bmp, BITMAP);
        I2S_writeBlock(0, 120, width - 1, 120+height - 1, &bmp, BITMAP);
        I2S_writeBlock(120, 120, 120+width - 1, 120+height - 1, &bmp, BITMAP);
        //I2S_writeBlock(0, 0, 2, 10, &bmp, BITMAP);
    }
    while(1);


    drawSquare_I2S(0, 0, 239, 239, 0x001f);
    drawSquare_I2S(0, 0, 239, 239, 0xf800);


    drawMono_I2S(0, 0, 239, 239, frame, 0x001f, 0xf800);
    //while(1);
        drawBitmap_I2S(120, 0, 120 + width - 1, height - 1, data);
    //    drawBitmap_I2S(0, 0, width - 1, height - 1, data);
        //drawBitmap_I2S(120, 0, 120 + width - 1, height - 1, data);
        while(1);
    while(1) {
        nrf_delay_ms(1000);
        drawBitmap_I2S(0, 0, width - 1, height - 1, data);
        //drawBitmap_I2S(120, 120, 120 + width - 1, 120 + height - 1, data);
        //drawBitmap_I2S(120, 0, 120 + width - 1, height - 1, data);
        //drawBitmap_I2S(0, 120, width - 1, 120 + height - 1, data);
        //drawSquare_I2S(0, 0, 120, 120, 0xf800);
    }

}
