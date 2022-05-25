#include <nrf.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>
#include <assert.h>
#include "I2S.h"
#include "display_defines.h"
#include "display.h"

void SPIM_init() {
    ////////////////
    // SPIM setup //
    ////////////////
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    NRF_SPIM0->PSEL.SCK  = LCD_SCK;
    NRF_SPIM0->PSEL.MOSI = LCD_MOSI;
    NRF_SPIM0->PSEL.MISO = LCD_MISO;

    NRF_SPIM0->CONFIG = (SPIM_CONFIG_ORDER_MsbFirst  << SPIM_CONFIG_ORDER_Pos)|
        (SPIM_CONFIG_CPOL_ActiveLow << SPIM_CONFIG_CPOL_Pos) |
        (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);

    NRF_SPIM0->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8 << SPIM_FREQUENCY_FREQUENCY_Pos;
}

void SPIM_enable(bool enabled) {
    if (enabled)
        NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    else {
        NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
        nrf_gpio_cfg_output(LCD_SCK);
        nrf_gpio_pin_write(LCD_SCK,1);
    }
}

// send one byte over spi (using SPIM to simplify the init commands)
void SPIM_send(bool mode, uint8_t byte) {
    if (mode)
        NRF_GPIOTE->TASKS_SET[1] = 1;
    else
        NRF_GPIOTE->TASKS_CLR[1] = 1;

    NRF_SPIM0->TXD.MAXCNT = 1;
    NRF_SPIM0->TXD.PTR = (uint32_t)&byte;

    NRF_SPIM0->EVENTS_ENDTX = 0;
    NRF_SPIM0->EVENTS_ENDRX = 0;
    NRF_SPIM0->EVENTS_END = 0;

    NRF_SPIM0->TASKS_START = 1;
    while(NRF_SPIM0->EVENTS_ENDTX == 0) __NOP();
    while(NRF_SPIM0->EVENTS_END == 0) __NOP();
    NRF_SPIM0->TASKS_STOP = 1;
    while (NRF_SPIM0->EVENTS_STOPPED == 0) __NOP();
}

void display_set_backlight(uint8_t brightness) {
    nrf_gpio_cfg_output(LCD_BACKLIGHT_HIGH);
    if (brightness != 0) {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,0);
    } else {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,1);
    }
}

void display_init() {
    I2S_init();
    SPIM_init();

    nrf_gpio_cfg_output(LCD_MOSI);
    nrf_gpio_cfg_output(LCD_SCK);
    nrf_gpio_cfg_output(LCD_COMMAND);
    nrf_gpio_pin_write(LCD_SCK,1);
    nrf_gpio_cfg_output(LCD_SELECT);
    nrf_gpio_cfg_output(LCD_RESET);
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_gpio_pin_write(LCD_COMMAND,1);
    nrf_gpio_pin_write(LCD_RESET,1);
    display_set_backlight(1);
    nrf_delay_ms(1);

    ///////////////////
    // reset display //
    ///////////////////
    nrf_gpio_pin_write(LCD_RESET,0);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_RESET,1);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_SELECT,0);

    SPIM_enable(1);

    SPIM_send(0, CMD_SWRESET);
    SPIM_send(0, CMD_SLPOUT);

    SPIM_send(0, CMD_COLMOD);
    SPIM_send(1, COLOR_16bit);

    SPIM_send(0, CMD_MADCTL);
    SPIM_send(1, 0x00);

    SPIM_send(0, CMD_INVON); // for standard 16 bit colors
    SPIM_send(0, CMD_NORON);
    SPIM_send(0, CMD_DISPON);

    SPIM_enable(0);
}

void display_scroll(uint16_t TFA, uint16_t VSA, uint16_t BFA, uint16_t scroll_value) {
    SPIM_enable(1);

    SPIM_send(0, CMD_VSCRDEF);

    SPIM_send(1, TFA >> 8);
    SPIM_send(1, TFA & 0xff);

    SPIM_send(1, VSA >> 8);
    SPIM_send(1, VSA & 0xff);

    SPIM_send(1, BFA >> 8);
    SPIM_send(1, BFA & 0xff);

    SPIM_send(0, CMD_MADCTL);
    SPIM_send(1, 0x0);

    SPIM_send(0, CMD_VSCSAD);

    SPIM_send(1, scroll_value >> 8);
    SPIM_send(1, scroll_value & 0xff);

    SPIM_enable(0);
}

void invert(bool inverted) {
    SPIM_enable(1);

    if (inverted)
        SPIM_send(0, CMD_INVON);
    else
        SPIM_send(0, CMD_INVOFF);

    SPIM_enable(0);
}

void display_draw_rect(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
   I2S_SOLID_COLOR_t Color = {.color = color};

   I2S_writeBlock(x1, y1, x2, y2, &Color, SOLID_COLOR);
}

void display_draw_bitmap(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data) {
    I2S_BITMAP_t bmp = {.bitmap = data};

   I2S_writeBlock(x1, y1, x2, y2, &bmp, BITMAP);
}

void display_draw_mono(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data, uint16_t color_fg, uint16_t color_bg) {
    I2S_MONO_t mono = {.bitmap = data,
    .color_fg = color_fg,
    .color_bg = color_bg};

   I2S_writeBlock(x1, y1, x2, y2, &mono, MONO);
}
