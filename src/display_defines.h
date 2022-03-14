#pragma once
  
#define LCD_SCK             2
#define LCD_MOSI            3
#define LCD_MISO            4
#define LCD_SELECT         25
#define LCD_COMMAND        18
#define LCD_RESET          26
#define PIN_LRCK           29 // unconnected pin, but has to be set up for the I2S peripheral to work

#define CMD_COLMOD    0x3A
#define CMD_INVON     0x21
#define CMD_INVOFF    0x20
#define CMD_SWRESET   0x01
#define CMD_SLPOUT    0x11
#define CMD_DISPON    0x29
#define CMD_CASET     0x2A
#define CMD_RASET     0x2B
#define CMD_RAMWR     0x2C
#define CMD_MADCTL    0x36
#define CMD_NORON     0x13


#define LCD_BACKLIGHT_LOW  14
#define LCD_BACKLIGHT_MID  22
#define LCD_BACKLIGHT_HIGH 23

#define COLOR_16bit 0x05

