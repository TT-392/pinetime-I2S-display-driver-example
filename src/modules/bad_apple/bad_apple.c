#include <nrf.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#include "display.h"
#include "bad_apple_data.h"
#include "ringbuff.h"
#include "rtc.h"
#include "wdt.h"
#include "display_print.h"
#include "main_menu.h"

struct dataBlock {
    int x1;
    int y1;
    int x2; // relative coords from x1 and y1
    int y2;

    bool newFrame;
    bool flipped;
    bool eof;
    bool staticFrames;
    int staticAmount;

    uint8_t* bitmap;
};

bool reset = 0;
int bad_apple_getc(ringbuffer* buffer) {
    static bool next_byte_eof = 0;
    if (reset)
        next_byte_eof = 1;

    if (next_byte_eof) {
        next_byte_eof = 0;
        return -1;
    }

    uint8_t byte;
    int retval = ringbuff_getc(&byte, buffer);
    if (retval == 1) {
        bad_apple_fetch_and_decompress(18400, 0);
        ringbuff_getc(&byte, buffer);
    } if (retval == -1) {
        next_byte_eof = 1;
    }
    return byte;
}

static uint8_t BMPAlloc[7200];
struct dataBlock readBlock(ringbuffer* buffer) {
    struct dataBlock retval = {};

    retval.eof = 0;

    uint8_t byte = bad_apple_getc(buffer);
    uint8_t c = byte;

    retval.newFrame = c & 1;
    bool shortCoords = (c >> 2) & 1;
    retval.flipped = (c >> 1) & 1;
    retval.staticFrames = (c >> 3) & 1;

    if (retval.staticFrames) {
        byte = bad_apple_getc(buffer);
        retval.staticAmount = byte;
        return retval;
    }

    retval.x1 = bad_apple_getc(buffer);
    retval.y1 = bad_apple_getc(buffer);

    if (shortCoords) {
        c = bad_apple_getc(buffer);
        retval.x2 = c & 0xf;
        retval.y2 = (c >> 4) & 0xf;
    } else {
        retval.x2 = bad_apple_getc(buffer);
        retval.y2 = bad_apple_getc(buffer);
    }

    int blockSize = ((retval.x2+1) * (retval.y2+1) + 7) / 8; // bytes rounded up

    //retval.bitmap = (uint8_t*)malloc(sizeof(uint8_t) * blockSize);
    retval.bitmap = BMPAlloc;

    for (int i = 0; i < blockSize; i++) {
        retval.bitmap[i] = bad_apple_getc(buffer);
    }
    
    return retval;
}

void wait_for_next_frame(bool reset) {
    static int renderedFrames;
    if (reset)
        renderedFrames = 0;

    // counter increments at 32768 Hz, beware, will overflow after about 4 minutes.
    while (((uint64_t)NRF_RTC2->COUNTER * 15) / 16384 < renderedFrames)
        bad_apple_fetch_and_decompress(18400, 0);
    renderedFrames++;
}

void render_video() {
    NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
        2 << GPIOTE_CONFIG_POLARITY_Pos |
        13 << GPIOTE_CONFIG_PSEL_Pos;

    rtc_setup();

    ringbuffer *videobuffer = bad_apple_init();
    bool flipped = 1;

    wait_for_next_frame(1);

    while (1) {
        if (NRF_GPIOTE->EVENTS_IN[3]) {
            NRF_GPIOTE->EVENTS_IN[3] = 0;
            ringbuff_destroy(videobuffer);
            flip(1);
            drawSquare(0, 0, 239, 319, 0x0000);
            system_task(start, &main_menu);
            return;
        }

        wdt_feed();
        struct dataBlock data = readBlock(videobuffer);

        if (data.eof)
            break;

        if (data.staticFrames) {
            for (int i = 0; i < data.staticAmount; i++) {
                wait_for_next_frame(0);
            }
        } else {
            if (data.newFrame) {
                wait_for_next_frame(0);
            }

            if (data.flipped) {
                flipped = !flipped;
                flip(flipped);
            }

            char string[50];

            drawMono(data.x1, data.y1, data.x2+data.x1, data.y2+data.y1, data.bitmap, 0x0000, 0xffff);

        }
    }
    free(videobuffer);
    ringbuff_destroy(videobuffer);
    flip(1);
    drawSquare(0, 0, 239, 319, 0x0000);
    system_task(start, &main_menu);
}
