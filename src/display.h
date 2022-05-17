#pragma once 

void display_init();

// quick and dirty known working drawSquare
void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void drawBitmap_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data);

void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void drawMono_I2S(int x1, int y1, int x2, int y2, uint8_t* frame, uint16_t posColor, uint16_t negColor);

