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

// quick and dirty known working drawSquare
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

static inline void swapBytes(uint8_t *buffer, int size) {
    assert(size % 2 == 0);
    for (int i = 0; i < size / 2; i++) {
        uint8_t temp = buffer[i*2];
        buffer[i*2] = buffer[i*2 + 1];
        buffer[i*2 + 1] = temp;
    }
}

void I2S_RAMWR(uint8_t* data, int pixCount)  {
    // This I2S driver works by abusing the TXPTRUPD event as a PPI event to
    // either stop the transfer or toggle the CMD pin. This event happens
    // roughly 8.5 bytes before the end of a dma transfer, though, because half
    // a byte is seen as no byte, and the CMD pin is sampled at the end of a
    // byte, in practice, the action taken on EVENTS_TXPTRUPD has effect 9
    // bytes (EVENTLEAD) before the end of the dma transfer. As an example of how
    // this is done, take the following I2S transfer of 7 pixels:
    //
    // start[12] = {0x00, 0x00, RAMWR, pix1.1, pix1.2, pix2.1, pix2.2, pix3.1, pix3.2, pix4.1, pix4.2, pix5.1}
    //                                   ^ *1
    // buffer[16] = {pix5.2, pix6.1, pix6.2, pix7.1, pix7.2, pix1.1, pix1.2, dummy, dummy, dummy, dummy, dummy, dummy, dummy, dummy, dummy}
    //                                                         ^ *3            ^ *2
    //
    // *1: The first TXPTRUPD event, the CMD pin will be set high here
    // *2: The second TXPTRUPD event, the I2S transfer will be stopped here
    // *3: The I2S dma transfer has to be a multiple of 4 bytes, therefore an
    // uneven number ends with the first pixel repeated
    //
    // More notes:
    //
    // Because the I2S doesn't do 16MHz 8 bit transfers, it has to have a word
    // size of 16 bytes at minimum, and, because of messed up endianness, the
    // buffers in the example will need to have their endianness swapped by
    // swapBytes() before the dma transfer.
    //
    // In reality, the transfer has 8 bytes and 1 bit of zeroes in front of it,
    // therefore 7 bits are bitbanged at the start of the I2S transfer.
    // Something similar goes for the end of the transfer, but this can be
    // fixed by toggling the CS pin.

    assert(pixCount != 0);

    const int EVENTLEAD = 9;

    int buffSize = 16; // has to be divisable by 4

    // The buffers contain extra space for trailing dummy bytes
    uint8_t buffer1[buffSize + EVENTLEAD];
    uint8_t buffer2[buffSize + EVENTLEAD];
    uint8_t buffer3[buffSize + EVENTLEAD];
    uint8_t *buffer = buffer1;
    uint8_t start[12] = {0x00, 0x00, CMD_RAMWR};

    I2S_enable(1);

    NRF_I2S->EVENTS_TXPTRUPD = 0;
    NRF_I2S->EVENTS_STOPPED = 0;


    NRF_GPIOTE->TASKS_CLR[1] = 1; // The command pin is connected to this GPIOTE channel

    NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected;

    // bitbang 7 0's to fix bit offset
    for (int i = 0; i < 7; i++) {
        nrf_gpio_pin_write(LCD_SCK,1);
        nrf_gpio_pin_write(LCD_SCK,0);
    }

    NRF_I2S->PSEL.SCK = LCD_SCK << I2S_PSEL_SCK_PIN_Pos;


    // We can only send an even number of pixels, in the case of an uneven
    // number, the first pixel is repeated at the end of the transfer
    uint8_t lastPixel[2];

    if (pixCount % 2) {
        lastPixel[0] = data[0];
        lastPixel[1] = data[1];
    } else {
        lastPixel[0] = data[pixCount*2 - 2];
        lastPixel[1] = data[pixCount*2 - 1];
    }

    int byteCount = (pixCount + (pixCount%2)) * 2;

    int copyCount;

    // The max number of pixels in the start buffer is equal to EVENTLEAD
    // because the cmd pin is toggled that many bytes before the end of the
    // transfer
    if (byteCount <= EVENTLEAD) {
        copyCount = byteCount-2;
        memcpy(start + 3 + copyCount, lastPixel, 2);
    } else {
        copyCount = 9;
    }

    memcpy(start + 3, data, copyCount);

    byteCount -= EVENTLEAD;
    data += copyCount;

    bool first = true;
    while (byteCount + EVENTLEAD > 0) {
        int transferSize;

        assert(!((byteCount + EVENTLEAD) % 4));

        if (byteCount > 0) {
            if (byteCount <= buffSize) {
                copyCount = byteCount-2;
                memcpy(buffer + copyCount, lastPixel, 2);

                transferSize = byteCount + EVENTLEAD;
            } else {
                copyCount = buffSize;
                transferSize = buffSize;
            }

            memcpy(buffer, data, copyCount);
        } else {
            transferSize = byteCount + EVENTLEAD;
        }
        swapBytes(buffer, transferSize);

        if (first) {

            swapBytes(start, 12);

            NRF_I2S->RXTXD.MAXCNT = 12/4;
            NRF_I2S->TXD.PTR = (uint32_t)start;
            NRF_I2S->TASKS_START = 1;
            first = false;
        }

        while (!NRF_I2S->EVENTS_TXPTRUPD);
        NRF_I2S->EVENTS_TXPTRUPD = 0;

        NRF_PPI->CHENSET = 1 << PPI_GPIOTE_SET;

        assert(!(transferSize % 4));

        NRF_I2S->RXTXD.MAXCNT = transferSize / 4;
        NRF_I2S->TXD.PTR = (uint32_t)buffer;


        byteCount -= transferSize;
        data += transferSize;

        if (buffer == buffer1) buffer = buffer2;
        else if (buffer == buffer2) buffer = buffer3;
        else buffer = buffer1;
    }

    while (!NRF_I2S->EVENTS_TXPTRUPD);
    NRF_I2S->EVENTS_TXPTRUPD = 0;

    NRF_PPI->CHENSET = 1 << PPI_I2S_TASKS_STOP;
    while (!NRF_I2S->EVENTS_STOPPED);

    NRF_PPI->CHENCLR = 1 << PPI_GPIOTE_SET;
    NRF_PPI->CHENCLR = 1 << PPI_I2S_TASKS_STOP;

    I2S_enable(0);

    // Transfer ends on a random bit, therefore CS has to be toggled to reset the bit counter
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_gpio_pin_write(LCD_SELECT,0);
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

    I2S_RAMWR(data, (x2-x1+1) * (y2-y1+1));
}

uint16_t convertColor(uint8_t r, uint8_t g, uint8_t b) {
    return ((r >> 3) << 11 | (g >> 2) << 5 | (b >> 3));
}

int main() {
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


    drawSquare(0, 0, 239, 239, 0x0000);

    int width = 120, height = 120;

    uint8_t data[width*height*2];


    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int g = ((255*x)/width) ^ ((255*y)/height);
            int b = ((255*(width-x))/width) ^ ((255*y)/height);

            uint16_t color = convertColor(0, g, b);
            data[(x + y*width) * 2] = color >> 8;
            data[(x + y*width) * 2 + 1] = color & 0xff;
        }
    }

    drawBitmap_I2S(0, 0, width - 1, height - 1, data);
    drawBitmap_I2S(120, 120, 120 + width - 1, 120 + height - 1, data);
    drawBitmap_I2S(120, 0, 120 + width - 1, height - 1, data);
    drawBitmap_I2S(0, 120, width - 1, 120 + height - 1, data);

    while(1);
}

