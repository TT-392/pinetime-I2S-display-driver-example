#include <nrf.h>
#include "display.h"

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

int main() {
    display_init();

    drawSquare(0, 0, 239, 239, 0x0000);

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

    drawBitmap_I2S(0, 0, width - 1, height - 1, data);
    drawBitmap_I2S(120, 120, 120 + width - 1, 120 + height - 1, data);
    drawBitmap_I2S(120, 0, 120 + width - 1, height - 1, data);
    drawBitmap_I2S(0, 120, width - 1, 120 + height - 1, data);

    while(1);
}

