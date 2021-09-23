#include "display_print.h"
#include "battery_pine.h"
#include <stdio.h>

void battery_init () {
    battery_setup();
}

int battery_percent() {
    int flags;
    float voltage, percent;

    battery_read(&flags, &voltage, &percent);

#define averaging_buffer_length 15
    static int averaging_buffer_index = 0;
    static float averaging_buffer[averaging_buffer_length];

    averaging_buffer[averaging_buffer_index++] = percent;
    averaging_buffer_index %= averaging_buffer_length;
    double average_percentage = 0;

    for (int i = 0; i < averaging_buffer_length; i++) {
        average_percentage += averaging_buffer[i];
    }
    average_percentage /= averaging_buffer_length;

    static int percentage_schmitt = 0;
    static int prev_percentage_schmitt = 0;

    while ((int)average_percentage > percentage_schmitt && (int)average_percentage > prev_percentage_schmitt) {
        prev_percentage_schmitt = percentage_schmitt;
        percentage_schmitt++;
    }

    while ((int)average_percentage < percentage_schmitt && (int)average_percentage < prev_percentage_schmitt) {
        prev_percentage_schmitt = percentage_schmitt;
        percentage_schmitt--;
    }

    return percentage_schmitt;
}

void battery_draw_percent (int x, int y, uint16_t color_text, uint16_t color_bg) {
    char batteryString[5];
    sprintf(batteryString, "%3d%%", battery_percent());
    drawString (x, y, batteryString, color_text, color_bg);
}
