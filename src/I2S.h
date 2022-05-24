#pragma once
#include <stdlib.h>
#include <stdbool.h>

typedef struct {
    uint16_t color;
    size_t pixCount;
} I2S_SOLID_COLOR_t;

typedef struct {
    uint8_t *bitmap;
    size_t pixCount;
} I2S_BITMAP_t;

typedef struct {
    uint8_t *bitmap;
    size_t pixCount;
    uint16_t color_fg;
    uint16_t color_bg;
} I2S_MONO_t;

enum transfertype {BITMAP, MONO, SOLID_COLOR};

void I2S_init();

void I2S_enable(bool enabled);

void I2S_RAMWR(void* data, enum transfertype type);

void I2S_RAMWR_COLOR(uint16_t color, int pixCount);

void I2S_writeBlock(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, void *data, enum transfertype type);

