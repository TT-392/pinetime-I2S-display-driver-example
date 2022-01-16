// TODO: test for memory leaks
#include <nrf.h>
#include <nrf_delay.h>
#include <nrf_gpio.h>
#include "i2s.h"
#include "display_defines.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define PIN_SCK    (2)
#define PIN_LRCK   (29) // unconnected pin, but has to be set up for the I2S peripheral to work
#define PIN_SDOUT  (3)

void I2S_init() {
    nrf_gpio_cfg_output(18);
    nrf_gpio_cfg_output(PIN_SCK);

    NRF_I2S->CONFIG.MCKFREQ = I2S_CONFIG_MCKFREQ_MCKFREQ_32MDIV2 << I2S_CONFIG_MCKFREQ_MCKFREQ_Pos;
    NRF_I2S->CONFIG.RATIO = I2S_CONFIG_RATIO_RATIO_32X << I2S_CONFIG_RATIO_RATIO_Pos;
    NRF_I2S->CONFIG.TXEN = (I2S_CONFIG_TXEN_TXEN_ENABLE << I2S_CONFIG_TXEN_TXEN_Pos);
    NRF_I2S->CONFIG.MODE = I2S_CONFIG_MODE_MODE_MASTER << I2S_CONFIG_MODE_MODE_Pos;
    // Has to be 16 Bit for it to be 16MHz
    NRF_I2S->CONFIG.SWIDTH = I2S_CONFIG_SWIDTH_SWIDTH_16BIT << I2S_CONFIG_SWIDTH_SWIDTH_Pos;
    NRF_I2S->CONFIG.FORMAT = I2S_CONFIG_FORMAT_FORMAT_I2S;
    NRF_I2S->CONFIG.CHANNELS = 0 << I2S_CONFIG_CHANNELS_CHANNELS_Pos;

    NRF_I2S->PSEL.SCK = (PIN_SCK << I2S_PSEL_SCK_PIN_Pos); 
    NRF_I2S->PSEL.LRCK = (PIN_LRCK << I2S_PSEL_LRCK_PIN_Pos); 
    NRF_I2S->PSEL.SDOUT = (PIN_SDOUT << I2S_PSEL_SDOUT_PIN_Pos);

    // setup CMD pin
    NRF_GPIOTE->CONFIG[1] = GPIOTE_CONFIG_MODE_Task << GPIOTE_CONFIG_MODE_Pos |
        GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos |
        18 << GPIOTE_CONFIG_PSEL_Pos | 
        GPIOTE_CONFIG_OUTINIT_Low << GPIOTE_CONFIG_OUTINIT_Pos;

    NRF_GPIOTE->TASKS_CLR[1] = 1;

    NRF_PPI->CH[1].EEP = (uint32_t) &NRF_I2S->EVENTS_TXPTRUPD;
    NRF_PPI->CH[1].TEP = (uint32_t) &NRF_GPIOTE->TASKS_OUT[1];
    NRF_PPI->CHENSET = 1 << 1;

    NRF_I2S->INTEN = 0;
    NRF_I2S->INTENSET = I2S_INTENSET_TXPTRUPD_Msk | I2S_INTENSET_STOPPED_Msk;

    NVIC_SetPriority(I2S_IRQn, 0); 
    NVIC_ClearPendingIRQ(I2S_IRQn);
    NVIC_EnableIRQ(I2S_IRQn);


    NRF_I2S->ENABLE = 1;
}

typedef struct transfer {
    uint8_t *buffer; int sizeDiv4;
    enum display_byte_type type;
    enum display_byte_type type_during_transfer;
    struct transfer *next;
    struct transfer *previous;
} transfer_t;

static volatile transfer_t *active_transfer = NULL;
static volatile transfer_t *newest_transfer = NULL;
static int used_mem_div4 = 0;
static bool I2S_running = 0;

void I2S_add_data_to_ll(const uint8_t* data, int sizeDiv4, enum display_byte_type type) {
    transfer_t *previous_transfer = (transfer_t*)newest_transfer;
    newest_transfer = malloc(sizeof(transfer_t));
    newest_transfer->buffer = malloc(sizeof(uint8_t) * sizeDiv4*4);
    newest_transfer->sizeDiv4 = sizeDiv4;
    newest_transfer->next = NULL;
    newest_transfer->type = type;
    newest_transfer->previous = previous_transfer;

    if (active_transfer == NULL)
        active_transfer = newest_transfer;

    if (previous_transfer != NULL) {
        previous_transfer->next = (transfer_t*)newest_transfer;
        previous_transfer->buffer[previous_transfer->sizeDiv4*4 - 2] = data[0];
    }

    for (int i = 0; i < sizeDiv4*2 - 1; i++) {
        newest_transfer->buffer[i*2 + 1] = data[i*2 + 1];
        newest_transfer->buffer[i*2] = data[i*2 + 2];
    }
    newest_transfer->buffer[sizeDiv4*4 - 1] = data[sizeDiv4*4 - 1];


    if (newest_transfer->previous != NULL)
        newest_transfer->type_during_transfer = newest_transfer->previous->type;
    else newest_transfer->type_during_transfer = D_CMD;

    used_mem_div4 += sizeDiv4;
}

void I2S_add_data(const uint8_t* data, int sizeDiv4, enum display_byte_type type) {
    assert(sizeDiv4 > 1);
    if (sizeDiv4 >= 2)
        I2S_add_data_to_ll(data, 2, type);
    if (sizeDiv4 >= 3)
        I2S_add_data_to_ll(data + 2*4, sizeDiv4 - 2, type);
}

void I2S_add_end() {
    uint8_t dummy[12] = {0};
    I2S_add_data(dummy, 3, D_END);
}

void I2S_reset() {
    while (I2S_cleanup());
    active_transfer = NULL;
    newest_transfer = NULL;
    NRF_I2S->ENABLE = 1;

    // the amount of bits send by the I2S driver isn't a multiple of 8 (It has some trailing bits), so we have to trigger the CS pin to reset the bit count on the display
    nrf_gpio_pin_write(LCD_SELECT,1);
    nrf_delay_ms(1);
    nrf_gpio_pin_write(LCD_SELECT,0);
}

int I2S_cleanup() {
    if (I2S_running) {
        transfer_t *temp_transfer = (transfer_t*)active_transfer;

        if (temp_transfer->previous != NULL) {
            temp_transfer = temp_transfer->previous; // newly assigned pointer
            if (temp_transfer->previous != NULL) { 
                temp_transfer = temp_transfer->previous; // actually active transfer

                int levels_deep = 0;
                while (temp_transfer->previous != NULL) {
                    temp_transfer = temp_transfer->previous;

                    if (temp_transfer->buffer != NULL) {
                        used_mem_div4 -= temp_transfer->sizeDiv4;
                        free(temp_transfer->buffer);
                        temp_transfer->buffer = NULL;
                    }

                    levels_deep++;
                }

                for (int i = 0; i < levels_deep - 1; i++) {
                    temp_transfer = temp_transfer->next;
                    free(temp_transfer->previous);
                    temp_transfer->previous = NULL;
                }
            }
        }
    } else {
        transfer_t *oldest_transfer = (transfer_t*)newest_transfer;

        while (oldest_transfer->previous != NULL) {
            oldest_transfer = oldest_transfer->previous;
        }

        while (oldest_transfer->next != NULL) {
            oldest_transfer = oldest_transfer->next;
            free(oldest_transfer->previous->buffer);
            free(oldest_transfer->previous);
            oldest_transfer->previous = NULL;
        }
        
        free(oldest_transfer->buffer);
        free(oldest_transfer);
        
        used_mem_div4 = 0;
    }

    return used_mem_div4;
}

void I2SHandler() {
    NRF_I2S->TXD.PTR = (uint32_t)active_transfer->buffer;
    NRF_I2S->RXTXD.MAXCNT = active_transfer->sizeDiv4;

    if (active_transfer->type_during_transfer == D_DAT)
        NRF_PPI->CH[1].TEP = (uint32_t) &NRF_GPIOTE->TASKS_SET[1];
    else if (active_transfer->type_during_transfer == D_CMD)
        NRF_PPI->CH[1].TEP = (uint32_t) &NRF_GPIOTE->TASKS_CLR[1];
    else if (active_transfer->type_during_transfer == D_END) {
        NRF_PPI->CH[1].TEP = (uint32_t) &NRF_I2S->TASKS_STOP;
        I2S_running = 0;
    }

    if (active_transfer->next != NULL)
        active_transfer = active_transfer->next;
}

void I2S_IRQHandler(void) {
    if (NRF_I2S->EVENTS_TXPTRUPD) {
        NRF_I2S->EVENTS_TXPTRUPD = 0;
        I2SHandler();
    }

    if (NRF_I2S->EVENTS_STOPPED) {
        NRF_I2S->EVENTS_STOPPED = 0;
        NRF_I2S->ENABLE = 0; // If this isn't called, the interrupt keeps triggering for some reason
    }
}

void I2S_start() {
    NRF_I2S->PSEL.SCK = I2S_PSEL_SCK_CONNECT_Disconnected; 

    // bitbang 0's to fix bit offset
    for (int i = 0; i < 7; i++) {
        nrf_gpio_pin_write(PIN_SCK,1);
        __NOP(); // may not be neccesary
        nrf_gpio_pin_write(PIN_SCK,0);
    }

    NRF_I2S->PSEL.SCK = (PIN_SCK << I2S_PSEL_SCK_PIN_Pos);

    I2S_running = 1;

    I2SHandler();
    NRF_I2S->TASKS_START = 1;
}
