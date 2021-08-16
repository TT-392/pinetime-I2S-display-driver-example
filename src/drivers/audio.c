#include <nrf.h>
#include <nrf_gpio.h>
#include "nrf_timer.h"
#include "display.h"

static bool flip2 = 1;
void TIMER2_IRQHandler(void) {
    //display_backlight(flip2);
    nrf_gpio_pin_write(16, flip2);
    flip2 = !flip2;
    NRF_TIMER2->EVENTS_COMPARE[0] = 0;
    if (NRF_TIMER2->EVENTS_COMPARE[1]) {
        NRF_TIMER2->EVENTS_COMPARE[1] = 0;
        NRF_TIMER2->TASKS_CLEAR = 1;
    }
}

// 20Hz to 20kHz
void audio_init() {
    nrf_gpio_cfg_output(16);
    nrf_gpio_pin_write(16, 1);

    NVIC_SetPriority(TIMER2_IRQn, 15); // Lowes priority
    NVIC_ClearPendingIRQ(TIMER2_IRQn);
    NVIC_EnableIRQ(TIMER2_IRQn);

    NRF_TIMER2->MODE = 0 << TIMER_MODE_MODE_Pos; // timer mode
    NRF_TIMER2->BITMODE = 3 << TIMER_BITMODE_BITMODE_Pos; // 23 bit
    NRF_TIMER2->PRESCALER = 0 << TIMER_PRESCALER_PRESCALER_Pos; // 1 MHz

    NRF_TIMER2->INTENSET = 1 << TIMER_INTENSET_COMPARE0_Pos;
    NRF_TIMER2->INTENSET = 1 << TIMER_INTENSET_COMPARE1_Pos;
}

void audio_set_freq(int frequency, float volume) {
    NRF_TIMER2->TASKS_STOP = 1;

    if (volume != 0) {
        NRF_TIMER2->CC[0] = ((16000000/frequency) / 13)* volume;
        NRF_TIMER2->CC[1] = 16000000/frequency;
        NRF_TIMER2->EVENTS_COMPARE[1] = 0;
        NRF_TIMER2->TASKS_CLEAR = 1;
        flip2 = 1;
        NRF_TIMER2->TASKS_START = 1;
    }
}
