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
    nrf_gpio_cfg_output(LCD_BACKLIGHT_HIGH);
    nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,0);
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

    SPIM_send (0, CMD_SWRESET);
    SPIM_send (0, CMD_SLPOUT);

    SPIM_send (0, CMD_COLMOD);
    SPIM_send (1, COLOR_16bit);

    SPIM_send (0, CMD_MADCTL);
    SPIM_send (1, 0x00);

    SPIM_send (0, CMD_INVON); // for standard 16 bit colors
    SPIM_send (0, CMD_NORON);
    SPIM_send (0, CMD_DISPON);

    SPIM_enable(0);
}

// quick and dirty known working drawSquare
void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    SPIM_enable(1);

    SPIM_send(0, CMD_CASET);

    SPIM_send(1, x1 >> 8);
    SPIM_send(1, x1 & 0xff);

    SPIM_send(1, x2 >> 8);
    SPIM_send(1, x2 & 0xff);

    SPIM_send(0, CMD_RASET);

    SPIM_send(1, y1 >> 8);
    SPIM_send(1, y1 & 0xff);

    SPIM_send(1, y2 >> 8);
    SPIM_send(1, y2 & 0xff);

    SPIM_send(0, CMD_RAMWR);

    int area = (x2-x1+1)*(y2-y1+1);
    int pixel = 0;

    while (pixel != area) {
        SPIM_send(1, color >> 8);
        SPIM_send(1, color & 0xff);

        pixel++;
    }
    SPIM_enable(0);
}

void drawBitmap_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t *data) {
    SPIM_enable(1);

    SPIM_send(0, CMD_CASET);

    SPIM_send(1, x1 >> 8);
    SPIM_send(1, x1 & 0xff);

    SPIM_send(1, x2 >> 8);
    SPIM_send(1, x2 & 0xff);

    SPIM_send(0, CMD_RASET);

    SPIM_send(1, y1 >> 8);
    SPIM_send(1, y1 & 0xff);

    SPIM_send(1, y2 >> 8);
    SPIM_send(1, y2 & 0xff);

    SPIM_enable(0);

    I2S_RAMWR_bitmap(data, (x2-x1+1) * (y2-y1+1));
}

#define binInd(buff, ind) ((buff[ind / 8] >> (ind % 8)) & 1)

void drawMono_I2S(int x1, int y1, int x2, int y2, uint8_t* frame, uint16_t posColor, uint16_t negColor) {
    SPIM_enable(1);
    size_t byteCount = 2 * (x2-x1+1) * (y2-y1+1);
    size_t chunkSize = 1048;
    assert(chunkSize > 480);

    columnWidth = x2-x1+1;
    chunkSize -= chunkSize % 
    uint8_t buffer[chunkSize];
    
    for (int i = 0; i < chunkSize; i += 2) {
        uint16_t color = binInd(frame, i) ? posColor : negColor;
        buffer[i] = color & 0xff;
        buffer[i] = color << 8;
        //*((uint16_t*)&buffer[i]) = color;
    }

    drawBitmap_I2S(x1, y1, x2, y2, buffer);
}

void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    SPIM_enable(1);

    SPIM_send(0, CMD_CASET);

    SPIM_send(1, x1 >> 8);
    SPIM_send(1, x1 & 0xff);

    SPIM_send(1, x2 >> 8);
    SPIM_send(1, x2 & 0xff);

    SPIM_send(0, CMD_RASET);

    SPIM_send(1, y1 >> 8);
    SPIM_send(1, y1 & 0xff);

    SPIM_send(1, y2 >> 8);
    SPIM_send(1, y2 & 0xff);

    SPIM_enable(0);

    I2S_RAMWR_solid_color(color, (x2-x1+1) * (y2-y1+1));
}
