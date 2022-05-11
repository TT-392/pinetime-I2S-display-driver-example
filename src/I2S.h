#pragma once

void I2S_init();

void I2S_enable(bool enabled);

void I2S_RAMWR(uint8_t* data, int pixCount);

void I2S_RAMWR_COLOR(uint16_t color, int pixCount);
