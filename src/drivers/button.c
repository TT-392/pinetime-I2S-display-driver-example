#include <stdbool.h>

void button_init() {
    NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
        2 << GPIOTE_CONFIG_POLARITY_Pos |
        13 << GPIOTE_CONFIG_PSEL_Pos;
}

bool button_check() {
    if (NRF_GPIOTE->EVENTS_IN[3]) {
        NRF_GPIOTE->EVENTS_IN[3] = 0;
        return 1;
    }
    else return 0;
}
