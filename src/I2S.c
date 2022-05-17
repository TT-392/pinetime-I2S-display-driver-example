#include <nrf.h>
#include <nrf_gpio.h>
#include "display_defines.h"
#include <assert.h>
#include <string.h>

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

    NRF_GPIOTE->TASKS_CLR[1] = 1;

    NRF_PPI->CH[1].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[1].TEP = (uint32_t) &NRF_I2S->TASKS_STOP;
    NRF_PPI->CH[2].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[2].TEP = (uint32_t) &NRF_GPIOTE->TASKS_SET[1];
}

#define PPI_I2S_TASKS_STOP 1
#define PPI_GPIOTE_SET 2

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
static size_t active_buffer_size;
static size_t active_buffer_index;

static inline void init_fill_buffer(uint8_t *input_buffer, size_t size) {
    active_buffer = input_buffer;    
    active_buffer_index = 0;
    active_buffer_size = size;
}

static inline void fill_buffer(volatile uint8_t *buffer, size_t size) {
    if (size < active_buffer_size - active_buffer_index) {
        memcpy((uint8_t*)buffer, active_buffer + active_buffer_index, size);
        active_buffer_index += size;
    } else {
        memcpy((uint8_t*)buffer, active_buffer + active_buffer_index, active_buffer_size - active_buffer_index);

        buffer += active_buffer_size - active_buffer_index;
        size -= active_buffer_size - active_buffer_index;
        active_buffer_index = 0;

        fill_buffer(buffer, size);
        /*
        assert(size < active_buffer_size - active_buffer_index);
        memcpy((uint8_t*)buffer, active_buffer + active_buffer_index, size);
        active_buffer_index += size;
        */
    }
}

void I2S_RAMWR_COLOR(uint16_t color, int pixCount) {
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

    volatile uint8_t buffer[4] = {color & 0xff,
        color >> 8, color & 0xff,
        color >> 8};

    volatile uint8_t start[12] = {0x00, 0x00, CMD_RAMWR,
        color >> 8, color & 0xff,
        color >> 8, color & 0xff,
        color >> 8, color & 0xff,
        color >> 8, color & 0xff,
        color >> 8};

    swapBytes(start, 12);
    swapBytes(buffer, 4);

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


    NRF_I2S->RXTXD.MAXCNT = sizeof(start)/4;
    NRF_I2S->TXD.PTR = (uint32_t)start;
    NRF_I2S->TASKS_START = 1;

    while (!NRF_I2S->EVENTS_TXPTRUPD);
    NRF_I2S->EVENTS_TXPTRUPD = 0;

    NRF_PPI->CHENSET = 1 << PPI_GPIOTE_SET;

    NRF_I2S->TXD.PTR = (uint32_t)buffer;
    NRF_I2S->RXTXD.MAXCNT = 1;


    // The amount of 4 byte chunks after the cmd pin was taken high until the transfer ends
    for (int i = 0; i < ((pixCount + 1) / 2); i++) {
        while (!NRF_I2S->EVENTS_TXPTRUPD);
        NRF_I2S->EVENTS_TXPTRUPD = 0;
    }
    

    NRF_PPI->CHENSET = 1 << PPI_I2S_TASKS_STOP;
    while (!NRF_I2S->EVENTS_STOPPED);

    NRF_PPI->CHENCLR = 1 << PPI_GPIOTE_SET;
    NRF_PPI->CHENCLR = 1 << PPI_I2S_TASKS_STOP;

    I2S_enable(0);

    // Transfer ends on a random bit, therefore CS has to be toggled to reset the bit counter
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}

static inline void wait_for_txptrupd() {
    while (!NRF_I2S->EVENTS_TXPTRUPD);
    NRF_I2S->EVENTS_TXPTRUPD = 0;
}

enum ppi_action {PPI_CMD_PIN_HIGH, PPI_END_TRANSFER};
static inline void set_action_on_next_event(enum ppi_action action) {
    switch (action) {
        case PPI_CMD_PIN_HIGH:
            NRF_PPI->CHENSET = 1 << PPI_GPIOTE_SET;
            break;
        case PPI_END_TRANSFER:
            NRF_PPI->CHENSET = 1 << PPI_I2S_TASKS_STOP;
            break;
    }
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

    NRF_GPIOTE->TASKS_CLR[1] = 1; // The command pin is connected to this GPIOTE channel

    NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected;

    // bitbang 7 0's to fix bit offset
    for (int i = 0; i < 7; i++) {
        nrf_gpio_pin_write(LCD_SCK,1);
        nrf_gpio_pin_write(LCD_SCK,0);
    }

    NRF_I2S->PSEL.SCK = LCD_SCK << I2S_PSEL_SCK_PIN_Pos;
}

void I2S_RAMWR(uint8_t* data, int pixCount)  {
    assert(pixCount != 0);

    int byteCount = (pixCount + pixCount%2) * 2;
    const int EVENTLEAD = 9;
    int buffSize = 16; // has to be divisable by 4

    // The buffers contain extra space for trailing dummy bytes
    volatile uint8_t buffer1[buffSize + EVENTLEAD];
    volatile uint8_t buffer2[buffSize + EVENTLEAD];
    volatile uint8_t buffer3[buffSize + EVENTLEAD];
    volatile uint8_t *buffer = buffer1;
    volatile uint8_t startbuff[12] = {0x00, 0x00, CMD_RAMWR};

    init_fill_buffer(data, pixCount * 2);

    int copyCount = sizeof(startbuff) - 3 < byteCount ? sizeof(startbuff) - 3 : byteCount;
    fill_buffer(startbuff + 3, copyCount);
    swapBytes(startbuff, sizeof(startbuff));
    byteCount -= sizeof(startbuff) - 3;

    if (byteCount > 0) {
        if (buffSize < byteCount) {
            fill_buffer(buffer, buffSize);
            swapBytes(buffer, buffSize);
            byteCount -= buffSize;
        } else {
            fill_buffer(buffer, byteCount);
            swapBytes(buffer, byteCount + EVENTLEAD);
        }
    }

    init_transfer();

    setup_next_txptr(startbuff, sizeof(startbuff));
    NRF_I2S->TASKS_START = 1;

    if (byteCount > buffSize) {
        wait_for_txptrupd();

        set_action_on_next_event(PPI_CMD_PIN_HIGH);
        setup_next_txptr(buffer, buffSize);

        while (byteCount > buffSize) {
            if (buffer == buffer1) buffer = buffer2;
            else if (buffer == buffer2) buffer = buffer3;
            else buffer = buffer1;

            copyCount = buffSize < byteCount ? buffSize : byteCount;
            fill_buffer(buffer, copyCount);
            swapBytes(buffer, copyCount);
            byteCount -= copyCount;

            wait_for_txptrupd();
            setup_next_txptr(buffer, buffSize);
        }
    }

    wait_for_txptrupd();

    set_action_on_next_event(PPI_CMD_PIN_HIGH);
    setup_next_txptr(buffer, byteCount + EVENTLEAD);

    wait_for_txptrupd();

    set_action_on_next_event(PPI_END_TRANSFER);
    while (!NRF_I2S->EVENTS_STOPPED);


    NRF_PPI->CHENCLR = 1 << PPI_GPIOTE_SET;
    NRF_PPI->CHENCLR = 1 << PPI_I2S_TASKS_STOP;

    I2S_enable(0);

    // Transfer ends on a random bit, therefore CS has to be toggled to reset the bit counter
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}

/*void I2S_RAMWR(uint8_t* data, int pixCount)  {
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
    volatile uint8_t buffer1[buffSize + EVENTLEAD];
    volatile uint8_t buffer2[buffSize + EVENTLEAD];
    volatile uint8_t buffer3[buffSize + EVENTLEAD];
    volatile uint8_t *buffer = buffer1;
    volatile uint8_t start[12] = {0x00, 0x00, CMD_RAMWR};

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
}*/
