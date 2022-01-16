#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "display_defines.h"
#include "display.h"
#include "i2s.h"
#include "nrf_assert.h"
#include <string.h>

#define PIN_LRCK   (29) // unconnected according to schematic
#define COLOR_18bit 0x06
#define COLOR_16bit 0x05
#define COLOR_12bit 0x03
static uint8_t colorMode = COLOR_16bit;

// placeholder for actual brightness control
void display_backlight(char brightness) {
    nrf_gpio_cfg_output(LCD_BACKLIGHT_HIGH);
    if (brightness != 0) {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,0);
    } else {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,1);
    }
}

// send one byte over spi (using SPIM to simplify the init commands)
void display_send(bool mode, uint8_t byte) {
    nrf_gpio_pin_write(LCD_COMMAND,mode);


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
    ////////////////
    // setup pins //
    ////////////////
    nrf_gpio_cfg_output(LCD_MOSI);
    nrf_gpio_cfg_output(LCD_SCK);
    nrf_gpio_cfg_input(LCD_MISO, NRF_GPIO_PIN_NOPULL);

    nrf_gpio_cfg_output(LCD_SELECT);
    nrf_gpio_cfg_output(LCD_COMMAND);
    nrf_gpio_cfg_output(LCD_RESET);

    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_gpio_pin_write(LCD_COMMAND,1);
    nrf_gpio_pin_write(LCD_RESET,1);

    nrf_gpio_cfg_output(LCD_COMMAND);


    ////////////////
    // SPIM setup //
    ////////////////
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    NRF_SPIM0->PSEL.SCK  = LCD_SCK;
    NRF_SPIM0->PSEL.MOSI = LCD_MOSI;
    NRF_SPIM0->PSEL.MISO = LCD_MISO;

    NRF_SPIM0->CONFIG = (SPIM_CONFIG_ORDER_MsbFirst  << SPIM_CONFIG_ORDER_Pos)|
                        (SPIM_CONFIG_CPOL_ActiveLow  << SPIM_CONFIG_CPOL_Pos) |
                        (SPIM_CONFIG_CPHA_Trailing   << SPIM_CONFIG_CPHA_Pos);

    NRF_SPIM0->FREQUENCY = SPIM_FREQUENCY_FREQUENCY_M8 << SPIM_FREQUENCY_FREQUENCY_Pos;
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;


    ///////////////////
    // reset display //
    ///////////////////
    nrf_gpio_pin_write(LCD_RESET,0);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_RESET,1);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_SELECT,0);


    display_send (0, CMD_SWRESET);
    display_send (0, CMD_SLPOUT);

    display_send (0, CMD_COLMOD);
    display_send (1, COLOR_16bit);

    display_send (0, CMD_MADCTL); 
    display_send (1, 0x00);

    display_send (0, CMD_INVON); // for standard 16 bit colors
    display_send (0, CMD_NORON);
    display_send (0, CMD_DISPON);



    // disable SPIM and start I2S
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    I2S_init();
}

void drawBitmap_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t* bitmap) {
    // We can only flip the command pin every 8 bytes, therefore we need a bunch of NOP's (0x00)
    uint8_t CASET[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_CASET};
    uint8_t coords1[8] = {x1 >> 8, x1 & 0xff, x2 >> 8, x2 & 0xff, 0x00, 0x00, 0x00, 0x00};
    
    uint8_t RASET[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_RASET};
    uint8_t coords2[8] = {y1 >> 8, y1 & 0xff, y2 >> 8, y2 & 0xff, 0x00, 0x00, 0x00, 0x00};

    uint8_t RAMWR[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_RAMWR};
    uint8_t FRAME[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    int area = (x2-x1+1)*(y2-y1+1);

    I2S_add_data(CASET, 2, D_CMD);
    I2S_add_data(coords1, 2, D_DAT);
    I2S_add_data(RASET, 2, D_CMD);
    I2S_add_data(coords2, 2, D_DAT);
    I2S_add_data(RAMWR, 2, D_CMD);



    I2S_add_data(bitmap, (area > 128 ? 128 : area) / 2, D_DAT);


    if (area <= 128) {
        I2S_add_end();
        I2S_start();
    } else {
        I2S_start();

        area -= 128;
        int i = 1;
        while (area > 0) {
            I2S_add_data(bitmap + i * 256, (area > 128 ? 128 : area) / 2, D_DAT);
            while (I2S_cleanup() > 128);
            area -= 128;
            i++;
        }
        I2S_add_end();
    }


    I2S_reset();
}

void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    uint8_t CASET[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_CASET};
    uint8_t coords1[8] = {x1 >> 8, x1 & 0xff, x2 >> 8, x2 & 0xff, 0x00, 0x00, 0x00, 0x00};
    
    uint8_t RASET[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_RASET};
    uint8_t coords2[8] = {y1 >> 8, y1 & 0xff, y2 >> 8, y2 & 0xff, 0x00, 0x00, 0x00, 0x00};

    uint8_t RAMWR[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, CMD_RAMWR};
    uint8_t FRAME[8] = {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff};

    uint8_t colorBuff[256];
    for (int i = 0; i < 128; i++) {
        colorBuff[i*2] = color >> 8;
        colorBuff[i*2 + 1] = color & 0xff;
    }

    int area = (x2-x1+1)*(y2-y1+1);

    I2S_add_data(CASET, 2, D_CMD);
    I2S_add_data(coords1, 2, D_DAT);
    I2S_add_data(RASET, 2, D_CMD);
    I2S_add_data(coords2, 2, D_DAT);
    I2S_add_data(RAMWR, 2, D_CMD);
    I2S_add_data(colorBuff, 64, D_DAT);
    I2S_start();

    for (int i = 0; i < area / 128; i++) {
        I2S_add_data(colorBuff, 64, D_DAT);
        while (I2S_cleanup() > 128);
    }
    I2S_add_end();

    I2S_reset();
}
