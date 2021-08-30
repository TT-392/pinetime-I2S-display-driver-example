#include "calctime.h"
#include "display.h"
#include "display_print.h"
#include "core.h"
#include "main_menu.h"
#include "clock_pine.h"
#include "nrf.h"
#include "nrf_gpio.h"
#include "touch.h"
#include "date_adjust.h"
#include "settings.h"

struct process date_adjust = {
    .runExists = 1,
    .run = &date_adjust_run,
    .startExists = 1,
    .start = &date_adjust_init,
    .stopExists = 0,
    .event = &event_always
};

void date_adjust_init() {
    drawSquare(0, 0, 239, 239, 0x0000);

    NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
        2 << GPIOTE_CONFIG_POLARITY_Pos |
        13 << GPIOTE_CONFIG_PSEL_Pos;

    // maybe implement process dependencies
    //touch_init();
}


void date_adjust_run() {
    static int selected = 0;
    bool selectNext = 0;
    struct touchPoints touchPoint;
    touch_refresh(&touchPoint);

    int delta = 0;
    if (touchPoint.tab && touchPoint.touchY != 0) {
        if (touchPoint.touchY > touchPoint.touchX && touchPoint.touchY > (240-touchPoint.touchX))
            delta = -1;
        else if (touchPoint.touchY < touchPoint.touchX && touchPoint.touchY < (240-touchPoint.touchX))
            delta = 1;
        else if (touchPoint.touchY > touchPoint.touchX && touchPoint.touchY < (240-touchPoint.touchX)) {
            selected--;
            selectNext = 1;
        } else if (touchPoint.touchY < touchPoint.touchX && touchPoint.touchY > (240-touchPoint.touchX)) {
            selected++;
            selectNext = 1;
        }
    }

    if (selected > 5)
        selected = 0;
    if (selected < 0)
        selected = 5;
    
    datetime timeDelta = {0};

    switch (selected) {
    case 0:
        timeDelta.year += delta;
        break;
    case 1:
        timeDelta.month += delta;
        break;
    case 2:
        timeDelta.day += delta;
        break;
    case 3:
        timeDelta.hour += delta;
        break;
    case 4:
        timeDelta.minute += delta;
        break;
    case 5:
        timeDelta.second += delta;
        break;
    }

    static datetime prevTime = {
        .second = -1,
        .minute = -1,
        .hour = -1,
        .day = -1,
        .month = -1,
        .year = -1
    };

    long long int epochTime = clock_time();
    if (delta != 0) {
        epochTime = addTime(epochTime, timeDelta);
        set_time(epochTime);
    }
    datetime time = epochtotime(epochTime);

    char* monthStrings[12] = {"JAN","FEB","MAR","APR","MAY","JUN","JUL","AUG","SEP","OCT","NOV","DEC"};
    uint16_t selectedColor = 0xffff;
    uint16_t notSelectedColor = 0x8 + (0x10 << 5) + (0x8 << 11);

    char string[5];

    if (prevTime.year != time.year || selectNext) {
        sprintf(string, "%04d", time.year);
        drawStringResized (3.5*32/2, 0, string, selected == 0 ? selectedColor : notSelectedColor, 0x0000, 4);
    }

    if (prevTime.month != time.month || selectNext) {
        drawStringResized (1.5*32/2, 16*5, monthStrings[time.month - 1], selected == 1 ? selectedColor : notSelectedColor, 0x0000, 4);
    }

    if (prevTime.day != time.day || selectNext) {
        sprintf(string, "%02d", time.day);
        drawStringResized (1.5*32/2 + 4*8*4, 16*5, string, selected == 2 ? selectedColor : notSelectedColor, 0x0000, 4);
    }

    if (prevTime.hour != time.hour || selectNext) {
        sprintf(string, "%02d", time.hour);
        drawStringResized (0, 16*5*2, string, selected == 3 ? selectedColor : notSelectedColor, 0x0000, 4);
    }

    if (prevTime.minute != time.minute || selectNext) {
        sprintf(string, "%02d", time.minute);
        drawStringResized (22*4, 16*5*2, string, selected == 4 ? selectedColor : notSelectedColor, 0x0000, 4);
    }

    if (prevTime.second != time.second || selectNext) {
        sprintf(string, "%02d", time.second);
        drawStringResized (22*4*2, 16*5*2, string, selected == 5 ? selectedColor : notSelectedColor, 0x0000, 4);
    }
    prevTime = time;

    if (NRF_GPIOTE->EVENTS_IN[3]) {
        NRF_GPIOTE->EVENTS_IN[3] = 0;
        core_stop_process(&date_adjust);
        core_start_process(&settings);
    }
}
