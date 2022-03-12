#include <nrf.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>
#include "display_defines.h"
#include <string.h>
#include <assert.h>

//#define PIN_MCK    (13)
#define PIN_LRCK   (29) // unconnected pin, but has to be set up for the I2S peripheral to work

#define COLOR_18bit 0x06
#define COLOR_16bit 0x05
#define COLOR_12bit 0x03

void swapBytes(uint8_t *buffer, int size) {
    assert(size % 2 == 0);
    for (int i = 0; i < size / 2; i++) {
        uint8_t temp = buffer[i*2];
        buffer[i*2] = 0;
        buffer[i*2 + 1] = 0;
        //buffer[i*2] = buffer[i*2 + 1];
        //buffer[i*2 + 1] = temp;
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

void I2S_init() {
    NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;

    // setup CMD pin
    NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
        GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
        LCD_COMMAND << GPIOTE_CONFIG_PSEL_Pos | 
        GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;

    NRF_GPIOTE->TASKS_CLR[1] = 1;


    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;

    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_32X << I2S_CONFIG_RATIO_RATIO_Pos;


    // Enable transmission
    NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);


    // Master mode, 16Bit, left aligned
    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_RIGHT << I2S_CONFIG_ALIGN_ALIGN_Pos;
    // Format = I2S
    NRF_I2S->CONFIG.FORMAT = 0;


    // Use stereo 
    NRF_I2S->CONFIG.CHANNELS = 0 << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

    // Configure pins
    NRF_I2S->PSEL.SCK = (LCD_SCK << I2S_PSEL_SCK_PIN_Pos); 
    NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos); 
    NRF_I2S->PSEL.SDOUT = (LCD_MOSI << I2S_PSEL_SDOUT_PIN_Pos);

    NRF_PPI->CH[1].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[1].TEP = (uint32_t) &NRF_I2S->TASKS_STOP;
    NRF_PPI->CH[2].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[2].TEP = (uint32_t) &NRF_GPIOTE->TASKS_SET[1];
}

#define PPI_I2S_TASKS_STOP 1
#define PPI_GPIOTE_SET 2

void SPIM_enable(bool enabled) {
    if (enabled)
        NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;
    else {
        NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
        nrf_gpio_cfg_output(LCD_SCK);
        nrf_gpio_pin_write(LCD_SCK,1);
    }
}

void I2S_enable(bool enabled) {
    if (enabled)
        NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Enabled << I2S_ENABLE_ENABLE_Pos;
    else {
        NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected; 
        NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;
        nrf_gpio_pin_write(LCD_SCK,1);
    }
}

void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    SPIM_enable(1);

    /* setup display for writing */
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
    /**/

    int area = (x2-x1+1)*(y2-y1+1);
    int pixel = 0;

    while (pixel != area) {
        SPIM_send(1, color >> 8);
        SPIM_send(1, color & 0xff);
        
        pixel++;

    }
    SPIM_enable(0);
}

void I2S_RAMWR(uint8_t* data, int pixCount)  {
    assert(pixCount != 0);

    SPIM_enable(0);
    NRF_GPIOTE->TASKS_CLR[1] = 1;
    I2S_enable(1);

    NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected; 

    // bitbang 7 0's to fix bit offset
    for (int i = 0; i < 7; i++) {
        nrf_gpio_pin_write(LCD_SCK,1);
        __NOP(); // may not be necessary
        nrf_gpio_pin_write(LCD_SCK,0);
    }

    NRF_I2S->PSEL.SCK = (LCD_SCK << I2S_PSEL_SCK_PIN_Pos);

    NRF_I2S->EVENTS_TXPTRUPD = 0;
    NRF_I2S->EVENTS_STOPPED = 0;

    bool uneven = pixCount % 2;
    pixCount += uneven;
    int byteCount = pixCount*2;
    int offset = 0;

    // buffSize has to be divisible by 4
    // Actual buffers are 8 bytes bigger to allow for end bytes
    int buffSize = 16; 
    uint8_t buffer1[buffSize + 8];
    uint8_t buffer2[buffSize + 8];
    uint8_t *buffer = buffer1;
    uint8_t start[12] = {0x00, 0x00, 0x2C};
    uint8_t firstPixel[2] = {data[0], data[1]};

    int copyCount;
    if (byteCount <= 9) {
        copyCount = byteCount - uneven*2;
    } else {
        copyCount = 9;
    }

    memcpy(start + 3, data, copyCount);

    if (byteCount <= 9 && uneven)
        memcpy(start + 3 + copyCount, firstPixel, 2);

    byteCount -= 9;
    data += copyCount;


    bool first = true;
    while (byteCount + 9 > 0) {
        int MAXCNT;
        if (byteCount > 0) {
            if (byteCount <= buffSize) {
                copyCount = byteCount - uneven*2;
                MAXCNT = (byteCount+9) / 4;
            } else {
                copyCount = buffSize;
                MAXCNT = (buffSize) / 4;
            }

            memcpy(buffer, data, copyCount);

            if (byteCount <= buffSize && uneven)
                memcpy(buffer + copyCount, firstPixel, 2);

            swapBytes(buffer, MAXCNT*4);
        } else {
            MAXCNT = (byteCount+9) / 4;
        }

        if (first) {
            swapBytes(start, 12);

            NRF_I2S->RXTXD.MAXCNT = 3;
            NRF_I2S->TXD.PTR = (uint32_t)start;

            NRF_I2S->TASKS_START = 1;
            first = false;
        }

        while (!NRF_I2S->EVENTS_TXPTRUPD);
        NRF_I2S->EVENTS_TXPTRUPD = 0;

        NRF_PPI->CHENSET = 1 << PPI_GPIOTE_SET;

        NRF_I2S->RXTXD.MAXCNT = MAXCNT;
        
        NRF_I2S->TXD.PTR = (uint32_t)buffer;

        byteCount -= MAXCNT * 4;
        data += MAXCNT * 4;
        if (buffer == buffer1) buffer = buffer2;
        else buffer = buffer1;
    }
    while (!NRF_I2S->EVENTS_TXPTRUPD);
    NRF_I2S->EVENTS_TXPTRUPD = 0;
    NRF_PPI->CHENSET = 1 << PPI_I2S_TASKS_STOP;

    while (!NRF_I2S->EVENTS_STOPPED);
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

    I2S_RAMWR(data, (x2-x1+1) * (y2-y1+1));


    // Since we are not updating the TXD pointer, the sine wave will play over and over again.
    // The TXD pointer can be updated after the EVENTS_TXPTRUPD arrives.
    while (1) {
        __WFE();
    }
}

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
  return ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

int main() {
    SPIM_enable(1);

    I2S_init();
    SPIM_init();

    //RAMWR_I2S("no", 2);

    /*
    uint8_t data[100];
    for (int i = 0; i < sizeof(data) / 2; i++) {
        data[i*2] = (i+1)*0x10 + 0;
        data[i*2+1] = (i+1)*0x10 + 1;
    }*/
    

//    I2S_RAMWR(data, 30);


    SPIM_enable(1);

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

    
    /*/
    ///////////////////
    // reset display //
    ///////////////////
    nrf_gpio_pin_write(LCD_RESET,0);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_RESET,1);
    nrf_delay_ms(200);
    nrf_gpio_pin_write(LCD_SELECT,0);

    SPIM_send (0, CMD_SWRESET);
    SPIM_send (0, CMD_SLPOUT);

    SPIM_send (0, CMD_COLMOD);
    SPIM_send (1, COLOR_16bit);

    SPIM_send (0, CMD_MADCTL); 
    SPIM_send (1, 0x00);

    SPIM_send (0, CMD_INVON); // for standard 16 bit colors
    SPIM_send (0, CMD_NORON);
    SPIM_send (0, CMD_DISPON);
    */

    SPIM_enable(0);

    //drawSquare(0, 0, 239, 239, 0x0000);
    
    int width = 100, height = 100;
    uint8_t data[20];

    for (int i = 0; i < sizeof(data); i++) {
        data[i] = i;
    }

    I2S_RAMWR(data, sizeof(data)/2);
    while(1);

    //for (int y = 0; y < height; y++) {
    //    for (int x = 0; x < width; x++) {
    //        int brightness = (x * width / 256) ^ (y * height / 256);
    //        uint16_t color = convertColor(brightness, brightness, brightness);
    //        color = 0xff00;
    //        data[(x + y*width) * 2] = color >> 8;
    //        data[(x + y*width) * 2 + 1] = color & 0xff;
    //    }
    //}

    drawBitmap_I2S(0, 0, width - 1, height - 1, data + 10);

    while(1);
}

