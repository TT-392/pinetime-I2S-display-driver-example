#include <nrf.h>
#include <nrf_gpio.h>
#include <nrf_delay.h>
#include "display_defines.h"
#include <assert.h>
#include <string.h>
#include "I2S.h"

void I2S_init() {
    NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;

    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_32X << I2S_CONFIG_RATIO_RATIO_Pos;
    NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);
    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_RIGHT << I2S_CONFIG_ALIGN_ALIGN_Pos;
    NRF_I2S->CONFIG.FORMAT = 0;
    NRF_I2S->CONFIG.CHANNELS = 0 << I2S_CONFIG_CHANNELS_CHANNELS_Pos;


    // setup CMD pin
    NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
        GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
        LCD_COMMAND << GPIOTE_CONFIG_PSEL_Pos |
        GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;


    NRF_TIMER3->MODE = 0 << TIMER_MODE_MODE_Pos; // timer mode
    NRF_TIMER3->BITMODE = 3 << TIMER_BITMODE_BITMODE_Pos; // 16 bit
    // Independent of prescaler setting the accuracy of the TIMER is equivalent
    // to one tick of the timer frequency fTIMER as illustrated in Figure 42:
    // Block schematic for timer/counter on page 234.
    NRF_TIMER3->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos; // 16 MHz

}

#define PPI_OFFSET 3

static int PPI_TIMER_CLEAR;
static int PPI_TIMER_START;

// Set up TIMER3 with CC's and PPI's to toggle the CMD pin and to trigger the stop task
static inline void PPI_setup(bool initial_state, uint32_t toggles[5], uint32_t stop) {
    int PPI_CHANNEL = PPI_OFFSET;
    NRF_PPI->CH[PPI_CHANNEL].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[PPI_CHANNEL].TEP = (uint32_t) &NRF_TIMER3->TASKS_CLEAR;
    NRF_PPI->CHENSET = 1 << PPI_CHANNEL;
    PPI_TIMER_CLEAR = PPI_CHANNEL;
    PPI_CHANNEL++;

    NRF_PPI->CH[PPI_CHANNEL].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[PPI_CHANNEL].TEP = (uint32_t) &NRF_TIMER3->TASKS_START;
    PPI_TIMER_START = PPI_CHANNEL;
    NRF_PPI->CHENSET = 1 << PPI_CHANNEL;
    PPI_CHANNEL++;

    if(initial_state)
        NRF_GPIOTE->TASKS_SET[1] = 1;
    else
        NRF_GPIOTE->TASKS_CLR[1] = 1;

    uint32_t offset = 4 + 8*8;

    for (int i = 0; i < 5; i++) {
        if (toggles[i] != 0) {
            NRF_TIMER3->CC[i] = offset + 8*toggles[i];

            NRF_PPI->CH[PPI_CHANNEL].EEP = (uint32_t) &NRF_TIMER3->EVENTS_COMPARE[i];
            NRF_PPI->CH[PPI_CHANNEL].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[1];

            NRF_PPI->CHENSET = 1 << (PPI_CHANNEL);
        } else {
            NRF_PPI->CHENCLR = 1 << (PPI_CHANNEL);
        }
        PPI_CHANNEL++;
    }

    NRF_TIMER3->CC[5] = offset + 8*stop;
    NRF_PPI->CH[PPI_CHANNEL].EEP = (uint32_t) &NRF_TIMER3->EVENTS_COMPARE[5];
    NRF_PPI->CH[PPI_CHANNEL].TEP = (uint32_t) &NRF_I2S->TASKS_STOP;
    NRF_PPI->CHENSET = 1 << PPI_CHANNEL;
}


void I2S_enable(bool enabled) {
    if (enabled) {
        NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Enabled << I2S_ENABLE_ENABLE_Pos;

        NRF_I2S->PSEL.SCK = (LCD_SCK << I2S_PSEL_SCK_PIN_Pos);
        NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos);
        NRF_I2S->PSEL.SDOUT = (LCD_MOSI << I2S_PSEL_SDOUT_PIN_Pos);
    } else {
        NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected;
        NRF_I2S->PSEL.LRCK = I2S_PSEL_SCK_CONNECT_Disconnected;
        NRF_I2S->PSEL.SDOUT = I2S_PSEL_SCK_CONNECT_Disconnected;

        NRF_I2S->ENABLE = I2S_ENABLE_ENABLE_Disabled << I2S_ENABLE_ENABLE_Pos;
        nrf_gpio_pin_write(LCD_SCK,1);
    }
}


static inline void swapBytes(volatile uint8_t *buffer, size_t size) {
    assert(size % 2 == 0);
    for (int i = 0; i < size / 2; i++) {
        uint8_t temp = buffer[i*2];
        buffer[i*2] = buffer[i*2 + 1];
        buffer[i*2 + 1] = temp;
    }
}

static uint8_t *active_buffer;
static size_t active_pixCount;
static size_t active_index;
static uint16_t color_fg;
static uint16_t color_bg;
static size_t active_index;
static enum transfertype active_transfertype;

#define binInd(buff, ind) ((buff[ind / 8] >> (ind % 8)) & 1)

static inline void init_bufferfiller(void *data, enum transfertype type) {
    switch (type) {
        case BITMAP:
            I2S_BITMAP_t *bmp_struct = (I2S_BITMAP_t*)data;
            active_buffer = bmp_struct->bitmap;
            active_pixCount = bmp_struct->pixCount * 2;
            break;
        case MONO:
            I2S_MONO_t *mono_struct = (I2S_MONO_t*)data;
            active_buffer = mono_struct->bitmap;
            active_pixCount = mono_struct->pixCount;
            color_fg = mono_struct->color_fg;
            color_bg = mono_struct->color_bg;
            break;
        case SOLID_COLOR:
            I2S_SOLID_COLOR_t *color_struct = (I2S_SOLID_COLOR_t*)data;
            active_pixCount = color_struct->pixCount;
            color_fg = color_struct->color;
            break;

    }

    active_index = 0;
    active_transfertype = type;
}

static inline int fill_buffer(volatile uint8_t *buffer, size_t size) {
    switch (active_transfertype) {
        case BITMAP:
            for (int i = 0; i < size; i += 2) {
                buffer[i] = (active_buffer + active_index)[1];
                buffer[i + 1] = (active_buffer + active_index)[0];

                active_index += 2;
                if (active_index >= active_pixCount){
                    return -1;
                }
            }
            return 0;
        case MONO:
            for (int i = 0; i < size; i += 2) {
                bool bit = binInd(active_buffer, active_index);

                static uint16_t color;
                color = bit ? color_fg: color_bg;

                buffer[i] = ((uint8_t*)&color)[0];
                buffer[i + 1] = ((uint8_t*)&color)[1];

                active_index++;
                if (active_index >= active_pixCount)
                    return -1;
            }
            return 0;
        case SOLID_COLOR:
            for (int i = 0; i < size; i += 2) {
                buffer[i] = ((uint8_t*)&color_fg)[0];
                buffer[i + 1] = ((uint8_t*)&color_fg)[1];

                active_index += 2;
                if (active_index >= active_pixCount){
                    return -1;
                }
            }
    }
}

static inline void wait_for_txptrupd() {
    while (!NRF_I2S->EVENTS_TXPTRUPD);
    NRF_I2S->EVENTS_TXPTRUPD = 0;
}

static inline void setup_next_txptr(volatile uint8_t* buffer, size_t size) {
    assert (size % 4 == 0);

    NRF_I2S->RXTXD.MAXCNT = size/4;
    NRF_I2S->TXD.PTR = (uint32_t)buffer;
}

static inline void init_transfer() {
    I2S_enable(1);

    NRF_I2S->EVENTS_TXPTRUPD = 0;
    NRF_I2S->EVENTS_STOPPED = 0;

    NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected;

    // bitbang 7 0's to fix bit offset
    for (int i = 0; i < 7; i++) {
        nrf_gpio_pin_write(LCD_SCK,1);
        nrf_gpio_pin_write(LCD_SCK,0);
    }

    NRF_I2S->PSEL.SCK = LCD_SCK << I2S_PSEL_SCK_PIN_Pos;
}

void I2S_writeBlock(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, void *data, enum transfertype type) {
    nrf_gpio_pin_write(LCD_SELECT,0);

    int pixCount = (x2-x1+1) * (y2-y1+1);

    if (type == MONO) {
        ((I2S_MONO_t*) data)->pixCount = pixCount;
    } else if (type == BITMAP) {
        ((I2S_BITMAP_t*) data)->pixCount = pixCount;
    } else if (type == SOLID_COLOR) {
        ((I2S_SOLID_COLOR_t*) data)->pixCount = pixCount;
    }

    // buffSize must be a multiple of 4 and >= 16, a smaller value means less
    // ram will be used (there are 3 buffers of size buffSize), and less time
    // will be needed when before the transfer (the first buffer is filled
    // before starting the transfer). Too small a value might cause stability
    // problems depending on compiler settings.
#define buffSize 16

    // The buffers contain extra space for trailing dummy bytes
    volatile uint8_t buffer1[buffSize] = {
        0,
        CMD_CASET,
        x1 >> 8,        // 2 (¬cmd high)
        x1 & 0xff,
        x2 >> 8,
        x2 & 0xff,
        CMD_RASET,      // 6 (¬cmd low)
        y1 >> 8,        // 7 (¬cmd high)
        y1 & 0xff,
        y2 >> 8,
        y2 & 0xff,
        CMD_RAMWR       // 11 (¬cmd low)
    };                  // 12 (¬cmd low)

    volatile uint8_t buffer2[buffSize];
    volatile uint8_t buffer3[buffSize];
    volatile uint8_t *buffer = buffer1;

    init_bufferfiller(data, type);

    PPI_setup(0, (uint32_t[5]) {2, 6, 7, 11, 12}, 2*pixCount + 12);
    swapBytes(buffer1, buffSize);

    int retval = fill_buffer(buffer + 12, buffSize - 12);

    init_transfer();

    setup_next_txptr(buffer, buffSize);
    NRF_I2S->TASKS_START = 1;

    while (retval != -1) {
        if (buffer == buffer1) buffer = buffer2;
        else if (buffer == buffer2) buffer = buffer3;
        else if (buffer == buffer3) buffer = buffer1;

        retval = fill_buffer(buffer, buffSize);

        wait_for_txptrupd();
        NRF_PPI->CHENCLR = 1 << PPI_TIMER_CLEAR;
        NRF_PPI->CHENCLR = 1 << PPI_TIMER_START;

        setup_next_txptr(buffer, buffSize);
    }

    while (!NRF_I2S->EVENTS_STOPPED);

    NRF_TIMER3->TASKS_STOP = 1;

    I2S_enable(0);

    // Transfer ends on a random bit, but this is fixed by making CS high
    nrf_gpio_pin_write(LCD_SELECT,1);
}
