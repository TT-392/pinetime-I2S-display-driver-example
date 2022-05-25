#pragma once 
#include <stdbool.h>
#include <stdint.h>

void display_init();

void display_set_backlight(uint8_t brightness);

void display_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

void display_draw_bitmap(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data);

void display_draw_mono(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data, uint16_t color_fg, uint16_t color_bg);

void display_set_backlight(uint8_t brightness);

void display_scroll(uint16_t TFA, uint16_t VSA, uint16_t BFA, uint16_t scroll_value);

void invert(bool inverted);

