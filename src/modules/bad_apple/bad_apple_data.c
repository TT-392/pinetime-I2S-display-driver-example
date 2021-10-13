#include "display.h"
#include "ringbuff.h"
#include "flash.h"
#include "lz4.h"

static ringbuffer *videobuffer;
// NOTE: possible to make this a sort of ringbuffer for a performance gain, might be negligible
static uint8_t spiflashbuffer[255];
static int spiflashbuffer_rptr;
static uint32_t flashAddr;

void spiflash_fetch() {
    display_pause();
    spiflash_read_data(flashAddr, spiflashbuffer, 255);
    flashAddr += 255;
    spiflashbuffer_rptr = 0;
    display_resume();
}

uint8_t spiflash_getc() {
    if (spiflashbuffer_rptr == 255)
        spiflash_fetch();

    return spiflashbuffer[spiflashbuffer_rptr++];
}

int bad_apple_fetch_and_decompress(uint64_t time, bool reset) {
    const uint64_t speed = 900000; // 8 Mbps = 1 MBps * 90% for margin
    const int timePerByte = (64000000 + speed - 1) / speed; // rounded up
    uint16_t byteCount = time / timePerByte;

    static enum lz4_retval lzstatus;
    static uint8_t input;
    static int first;
    if (reset) {
        lzstatus = LZ4_MOREDATA;
        first = 1;
        lz4_decompress(0, 0, 1);
    }

    if (first) {
        input = spiflash_getc();
        byteCount--;
        first = 0;
    }

    bool dataAdded = 0;
    while (byteCount > 0) {
        lzstatus = lz4_decompress(input, videobuffer, 0);
        if (lzstatus == LZ4_MOREDATA) {
            input = spiflash_getc();
            byteCount--;
            dataAdded = 1;
        } else if (lzstatus == LZ4_EOF)
            return 0;
        else
            return 1; // full buffer
    }
    return 0;
}

ringbuffer *bad_apple_init() {
    spiflashbuffer_rptr = 0;
    flashAddr = 0;

    spiflash_fetch();
    videobuffer = ringbuff_create(10000);

    ringbuff_destroy(videobuffer);
    videobuffer = ringbuff_create(10000);

    bad_apple_fetch_and_decompress(64000000, 1);

    return videobuffer;
}

