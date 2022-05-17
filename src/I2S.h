#pragma once

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

enum transfertype {BITMAP, MONO};

void I2S_init();

void I2S_enable(bool enabled);

void I2S_RAMWR(void* data, enum transfertype type);

void I2S_RAMWR_COLOR(uint16_t color, int pixCount);
