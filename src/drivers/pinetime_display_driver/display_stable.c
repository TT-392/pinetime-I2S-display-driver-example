#include "nrf.h"
#include "nrf_gpio.h"
#include "nrf_delay.h"
#include "display_defines.h"
#include "display.h"
#include "i2s.h"
#include "nrf_assert.h"
#include <string.h>

#define PIN_LRCK   (29) // unconnected according to schematic

#define ppi_set() NRF_PPI->CHENSET = 0xff; __disable_irq();// enable first 8 ppi channels
#define ppi_clr() NRF_PPI->CHENCLR = 0xff; __enable_irq(); // disable first 8 ppi channels

#define COLOR_18bit 0x06
#define COLOR_16bit 0x05
#define COLOR_12bit 0x03
static uint8_t colorMode = COLOR_16bit;
void display_init();

static task tasks[] = {{&display_init, start, 0}};

process display = {
    .taskCnt = 1,
    .tasks = tasks
};


// placeholder for actual brightness control see https://forum.pine64.org/showthread.php?tid=9378, pwm is planned
void display_backlight(char brightness) {
    nrf_gpio_cfg_output(LCD_BACKLIGHT_HIGH);
    if (brightness != 0) {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,0);
    } else {
        nrf_gpio_pin_write(LCD_BACKLIGHT_HIGH,1);
    }
}

// send one byte over spi
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

// send a bunch of bytes from buffer
void display_sendbuffer(bool mode, uint8_t* m_tx_buf, int m_length) {
    NRF_SPIM0->TXD.MAXCNT = m_length;
    NRF_SPIM0->TXD.PTR = (uint32_t)m_tx_buf;

    NRF_SPIM0->EVENTS_ENDTX = 0;
    NRF_SPIM0->EVENTS_ENDRX = 0;
    NRF_SPIM0->EVENTS_END = 0;

    NRF_SPIM0->TASKS_START = 1;
    while(NRF_SPIM0->EVENTS_ENDTX == 0) __NOP();
    while(NRF_SPIM0->EVENTS_END == 0) __NOP();
    NRF_SPIM0->TASKS_STOP = 1;
    while (NRF_SPIM0->EVENTS_STOPPED == 0) __NOP();
}

// send a bunch of bytes from buffer
void display_sendbuffer_noblock(uint8_t* m_tx_buf, int m_length) {
    NRF_SPIM0->TXD.MAXCNT = m_length;
    NRF_SPIM0->TXD.PTR = (uint32_t)&m_tx_buf[0];

    NRF_SPIM0->EVENTS_ENDTX = 0;
    NRF_SPIM0->EVENTS_ENDRX = 0;
    NRF_SPIM0->EVENTS_END = 0;

    NRF_SPIM0->TASKS_START = 1;
}

// this function must be called after display_sendbuffer_noblock has been called
// and before the next call of spim related functions. It will wait for spim to
// finish and will then stop spim0
void display_sendbuffer_finish() {
    while(NRF_SPIM0->EVENTS_ENDTX == 0) 
        __NOP();

    while(NRF_SPIM0->EVENTS_END == 0) 
        __NOP();

    NRF_SPIM0->TASKS_STOP = 1;
    while (NRF_SPIM0->EVENTS_STOPPED == 0)
        __NOP();
}

void cmd_enable(bool enabled) {
    if (enabled) {
        // create GPIOTE task to switch LCD_COMMAND pin
        NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
            GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
            LCD_COMMAND << GPIOTE_CONFIG_PSEL_Pos | 
            GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;
        ppi_set();
    } else {
        NRF_GPIOTE->CONFIG[1] = 0;
        ppi_clr();
        nrf_gpio_cfg_output(LCD_COMMAND);
    }
}

void set_colormode(uint8_t colormode) {
    cmd_enable(0);
    display_send (0, CMD_COLMOD);
    display_send (1, colormode);
    cmd_enable(1);
    colorMode = colormode;
}

void flip (bool flipped) {
    cmd_enable(0);
    if (flipped)
        display_send (0, CMD_INVON);
    if (!flipped)
        display_send (0, CMD_INVOFF);

    cmd_enable(1);
}

void cc_setup(int flip1, int flip2, int flip3, int flip4, int flip5, int flip6) {
    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    // offset from spim events started to first bit
    // cmd pin gets sampled at end of byte, so we flip it in the middle for better stability
    // 2 * 1 nibble = 8 = offset to get to middle of byte
    NRF_TIMER3->CC[0] = 5 + 8 + (8 * flip1 * 2);
    NRF_TIMER3->CC[1] = 5 + 8 + (8 * flip2 * 2);
    NRF_TIMER3->CC[2] = 5 + 8 + (8 * flip3 * 2);
    NRF_TIMER3->CC[3] = 5 + 8 + (8 * flip4 * 2);
    NRF_TIMER3->CC[4] = 5 + 8 + (8 * flip5 * 2);
    NRF_TIMER3->CC[5] = 5 + 8 + (8 * flip6 * 2);
} // TODO: I am pretty sure there is no reason to have the first flip at 0, this is a wasted CC and ppi


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


    ///////////////
    // spi setup //
    ///////////////
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



    ///////////////////////////
    // setup LCD_COMMAND PIN //
    ///////////////////////////
    nrf_gpio_cfg_output(LCD_COMMAND);


    NRF_TIMER3->MODE = 0 << TIMER_MODE_MODE_Pos; // timer mode
    NRF_TIMER3->BITMODE = 0 << TIMER_BITMODE_BITMODE_Pos; // 16 bit
    NRF_TIMER3->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos; // 16 MHz

    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    cc_setup(0,1,5,6,10,11);


    // PPI channels for toggeling pin
    for (int channel = 0; channel < 6; channel++) { 
        NRF_PPI->CH[channel].EEP = (uint32_t) &NRF_TIMER3->EVENTS_COMPARE[channel];
        if (channel % 2)
            NRF_PPI->CH[channel].TEP = (uint32_t) &NRF_GPIOTE->TASKS_SET[1];
        else 
            NRF_PPI->CH[channel].TEP = (uint32_t) &NRF_GPIOTE->TASKS_CLR[1];
    }


    NRF_PPI->CH[6].EEP = (uint32_t) &NRF_SPIM0->EVENTS_STARTED;
    NRF_PPI->CH[6].TEP = (uint32_t) &NRF_TIMER3->TASKS_CLEAR;

    NRF_PPI->CH[7].EEP = (uint32_t) &NRF_SPIM0->EVENTS_STARTED;
    NRF_PPI->CH[7].TEP = (uint32_t) &NRF_TIMER3->TASKS_START;

    //  NRF_PPI->CH[8].EEP = (uint32_t) &NRF_TIMER3->EVENTS_COMPARE[5];
    //  NRF_PPI->CH[8].TEP = (uint32_t) &NRF_TIMER3->TASKS_STOP;

    cmd_enable(1);
}

void display_pause() {
    cmd_enable(0);
    nrf_gpio_pin_write(LCD_SELECT,1);
}
void display_resume() {
    cmd_enable(1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}

void drawSquare(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    ppi_clr();
    cmd_enable(0);

    /* setup display for writing */
    display_send(0, CMD_CASET);

    display_send(1, x1 >> 8);
    display_send(1, x1 & 0xff);

    display_send(1, x2 >> 8);
    display_send(1, x2 & 0xff);

    display_send(0, CMD_RASET);

    display_send(1, y1 >> 8);
    display_send(1, y1 & 0xff);

    display_send(1, y2 >> 8);
    display_send(1, y2 & 0xff);

    display_send(0, CMD_RAMWR);
    /**/

    int area = (x2-x1+1)*(y2-y1+1);
    int pixel = 0;

    while (pixel != area) {
        display_send(1, color >> 8);
        display_send(1, color & 0xff);
        
        pixel++;

    }
    ppi_clr();
    cmd_enable(1);
}

void drawSquare_I2S(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t color) {
    ppi_clr();
    cmd_enable(0);
    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;

    I2S_init();

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


    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;

    cmd_enable(1);
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_delay_ms(1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}

void drawMono_I2S(int x1, int y1, int x2, int y2, uint8_t* frame, uint16_t posColor, uint16_t negColor) {
    ppi_clr();
    cmd_enable(0);

    /* setup display for writing */
    display_send(0, CMD_CASET);

    display_send(1, x1 >> 8);
    display_send(1, x1 & 0xff);

    display_send(1, x2 >> 8);
    display_send(1, x2 & 0xff);

    display_send(0, CMD_RASET);

    display_send(1, y1 >> 8);
    display_send(1, y1 & 0xff);

    display_send(1, y2 >> 8);
    display_send(1, y2 & 0xff);

    display_send(0, CMD_RAMWR);
    /**/

    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Disabled << SPIM_ENABLE_ENABLE_Pos;
    nrf_gpio_pin_write(LCD_COMMAND,1);

    for (int i = 0; i < 7; i++) {
        nrf_delay_ms(1);
        nrf_gpio_pin_write(LCD_SCK,1);
        nrf_delay_ms(1);
        nrf_gpio_pin_write(LCD_SCK,0);
    }

    // Enable transmission
    NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);

    // Ratio = 64 
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_32X << I2S_CONFIG_RATIO_RATIO_Pos;

    // Master mode, 16Bit, left aligned
    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.ALIGN = I2S_CONFIG_ALIGN_ALIGN_RIGHT << I2S_CONFIG_ALIGN_ALIGN_Pos;
    // Format = I2S
    NRF_I2S->CONFIG.FORMAT = 1 << 1;

    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;

    // Format = I2S
    NRF_I2S->CONFIG.FORMAT = 1;

    // Use stereo 
    NRF_I2S->CONFIG.CHANNELS = I2S_CONFIG_CHANNELS_CHANNELS_STEREO << I2S_CONFIG_CHANNELS_CHANNELS_Pos;
    

    // Configure pins
    NRF_I2S->PSEL.SCK = (LCD_SCK << I2S_PSEL_SCK_PIN_Pos); 
    NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos); 
    NRF_I2S->PSEL.SDOUT = (LCD_MOSI << I2S_PSEL_SDOUT_PIN_Pos);
    
  
    NRF_I2S->ENABLE = 1;

    // Configure data pointer
    uint8_t byte[0x3fff*2] = {0};
    
    NRF_I2S->TXD.PTR = (uint32_t)byte;
    NRF_I2S->RXTXD.MAXCNT = 0x3fff/2;

    NRF_I2S->TASKS_START = 1;

    int area = (x2-x1+1)*(y2-y1+1);

    for (int i = 0; i < (area + 2) / (2*NRF_I2S->RXTXD.MAXCNT); i++) {
        for (int pixel = 0; pixel < (0x3fff*2) / 2; pixel++) {
            if ((frame[(pixel + 3 + 0x3fff * i) / 8] >> (pixel + 3 + 0x3fff * i) % 8) & 1) {
                byte[pixel*2] = 0x00;
                byte[pixel*2 + 1] = 0x00;
            } else {
                byte[pixel*2] = 0xf8;
                byte[pixel*2 + 1] = 0x00;
            }
        }

        NRF_I2S->EVENTS_TXPTRUPD = 0;
        while (!NRF_I2S->EVENTS_TXPTRUPD) __NOP();
    }

    NRF_I2S->TASKS_STOP = 1;
    nrf_delay_us(1);
    NRF_I2S->ENABLE = 0;

    NRF_SPIM0->ENABLE = SPIM_ENABLE_ENABLE_Enabled << SPIM_ENABLE_ENABLE_Pos;

    cmd_enable(1);
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_delay_ms(1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}



void drawBitmap (uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint8_t* bitmap) {
    ppi_clr();
    cmd_enable(0);

    /* setup display for writing */
    display_send(0, CMD_CASET);

    display_send(1, x1 >> 8);
    display_send(1, x1 & 0xff);

    display_send(1, x2 >> 8);
    display_send(1, x2 & 0xff);

    display_send(0, CMD_RASET);

    display_send(1, y1 >> 8);
    display_send(1, y1 & 0xff);

    display_send(1, y2 >> 8);
    display_send(1, y2 & 0xff);

    display_send(0, CMD_RAMWR);
    /**/

    int area = (x2-x1+1)*(y2-y1+1);
    int pixel = 0;

    while (pixel != area) {
        display_send(1, bitmap[pixel*2]);
        display_send(1, bitmap[pixel*2 + 1]);
        
        pixel++;

    }

    ppi_clr();
    cmd_enable(1);
}

void drawMono(int x1, int y1, int x2, int y2, uint8_t* frame, uint16_t posColor, uint16_t negColor) {
    ppi_clr();
    cmd_enable(0);

    /* setup display for writing */
    display_send(0, CMD_CASET);

    display_send(1, x1 >> 8);
    display_send(1, x1 & 0xff);

    display_send(1, x2 >> 8);
    display_send(1, x2 & 0xff);

    display_send(0, CMD_RASET);

    display_send(1, y1 >> 8);
    display_send(1, y1 & 0xff);

    display_send(1, y2 >> 8);
    display_send(1, y2 & 0xff);

    display_send(0, CMD_RAMWR);
    /**/

    int area = (x2-x1+1)*(y2-y1+1);
    int pixel = 0;

    while (pixel != area) {
        if ((frame[pixel / 8] >> pixel % 8) & 1) {
            display_send(1, posColor >> 8);
            display_send(1, posColor & 0xff);
        } else {
            display_send(1, negColor >> 8);
            display_send(1, negColor & 0xff);
        }
        
        pixel++;

    }
    ppi_clr();
    cmd_enable(1);
}

void display_scroll(uint16_t TFA, uint16_t VSA, uint16_t BFA, uint16_t scroll_value) {
    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    cc_setup(0,1,7,8,9,10);
    ppi_set();

    uint8_t byteArray[12];
    byteArray[0] = CMD_VSCRDEF;

    byteArray[1] = TFA >> 8;
    byteArray[2] = TFA & 0xff;

    byteArray[3] = VSA >> 8;
    byteArray[4] = VSA & 0xff;

    byteArray[5] = BFA >> 8;
    byteArray[6] = BFA & 0xff;

    byteArray[7] = CMD_MADCTL;
    byteArray[8] = 0x0;

    byteArray[9] = CMD_VSCSAD;

    byteArray[10] = scroll_value >> 8;
    byteArray[11] = scroll_value & 0xff;

    display_sendbuffer(0, byteArray, 12);

    /**/

    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    cc_setup(0,1,5,6,10,11);
    ppi_clr();
}

void partialMode(uint16_t PSL, uint16_t PEL) {
    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    cc_setup(0,1,2,3,7,50);
    ppi_set();

    uint8_t byteArray[12];
    byteArray[0] = CMD_MADCTL;
    byteArray[1] = 0;

    byteArray[2] = CMD_PTLAR;

    byteArray[3] = PSL >> 8;
    byteArray[4] = PSL & 0xff;

    byteArray[5] = PEL >> 8;
    byteArray[6] = PEL & 0xff;

    byteArray[7] = CMD_PTLON;

    /**/

    // the following CC setup will cause byte 0, 5 and 10 
    // of any SPIM0 dma transfer to be treated as CMD bytes
    cc_setup(0,1,5,6,10,11);
    ppi_clr();
}
