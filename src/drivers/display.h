#pragma once
#include "stdint.h"
#include "stdbool.h"

/*
 * Function to init the display, 
 * this will initialize and use NRF_TIMER3 and the first 8 PPI channels
 */
void display_init();

/*
 * Function to draw a square
 */
void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);

/*
 * placeholder for actual brightness control, 0 = off and > 1 is 100%
 */
void display_backlight(char brightness);

/*
 * Function to draw a 1 bit bitdepth bitmap
 */




void flip (bool flipped);

void display_pause();
void display_resume();


void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color);
void drawBitmap_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t* bitmap);

