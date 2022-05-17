#pragma once 

void display_init();

// quick and dirty known working drawSquare
void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void drawBitmap_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data);

void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void drawMono_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data, uint16_t color_fg, uint16_t color_bg);
