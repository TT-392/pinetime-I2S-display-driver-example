#include "nrf_clock.h"
#include "nrf_timer.h"
#include "main_menu.h"
#include "display_print.h"
#include "steamLocomotive.h"
#include "display.h"
#include "system.h"
#include "time_event_handler.h"

void sl_init();
void sl_stop();
void sl_run();

static dependency dependencies[] = {{&display, running}, {&time_event_handler, running}};
static task tasks[] = {{&sl_init, start, 2, dependencies}, {&sl_run, run, 0}, {&sl_stop, stop, 0}};

process sl = {
    .taskCnt = 3,
    .tasks = tasks,
};

void sl_static(int x, int y, uint16_t color_text, uint16_t color_bg) {
    char* train[6] = {
    "      ++      +------", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--O========O~\\-+ ",
    " //// \\_/      \\_/   "
    };


    for (int i = 0; i < 6; i++) {
        drawString (x, y + i*16, train[i], color_text, color_bg);
    }
}

void sl_init() {
    drawSquare(0, 20, 239, 319, 0x0000);

    sl.trigger = create_time_event(8);
}

void sl_stop() {
    free_time_event(sl.trigger);
}


void sl_run() {
    int y = 65;
    uint16_t color_text = 0xffff;
    uint16_t color_bg = 0x0000;
    static int time = 0;

    static bool firstframe = 1;
    if (firstframe) {
    }


    char* trains[6][6] = {{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--/~O========O-+ ",
    " //// \\_/      \\_/   "
    },{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--/O========O\\-+ ",
    " //// \\_/      \\_/   "
    },{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--O========O~\\-+ ",
    " //// \\_/      \\_/   "
    },{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--/~\\------/~\\-+ ",
    " //// 0========0_/   "
    },{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--/~\\------/~\\-+ ",
    " //// \\0========0/   "
    },{
    "      ++      +------ ", 
    "      ||      |+-+ | ",
    "    /---------|| | | ",
    "   + ========  +-+ | ",
    "  _|--/~\\------/~\\-+ ",
    " //// \\_0========0   "
    }};


    for (int line = 0; line < 6; line++) {
        int x = (30-time)*8;
        int charY = y + line*16;
        char* string = trains[(time/2)%6][line];
        _Bool terminated = 0;
        int character = 1;

        while (!terminated) {
            if (string[character] == '\0') {
                terminated = 1;
            } else {
                if (((x + character*8) < 240 && (x + character*8) >= 0) // char out of display bounds
                && ((line > 3 && character > 6) || string[character-1] != string[character])) { // optimization
                    drawChar (x + character*8, charY, string[character], color_text, color_bg, 0);
                }
                character++;
            }
        }
    }
    time++;
    if (time >= (30+22)) {
        time = 0;
        //core_stop_process(&sl);
        //core_start_process(&main_menu);
    } 
}
