#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "display.h"
#include "display_print.h"
#include "touch.h"
#include "nrf_delay.h"
#include "breakout.h"
#include "main_menu.h"
#include "system.h"
#include "statusbar.h"
#include "time_event_handler.h"

void breakout_run();
void breakout_init();
void breakout_stop();

static dependency dependencies[] = {{&display, running}, {&touch, running}, {&time_event_handler, running}, {&statusbar, stopped}};
static task tasks[] = {{&breakout_init, start, 4, dependencies}, {&breakout_run, run, 0}, {&breakout_stop, stop, 0}};

process breakout = {
    .taskCnt = 3,
    .tasks = tasks,
};

struct Ball {
    uint8_t x;
    uint8_t y;
    int vx;
    int vy;
};

static const uint8_t bw = 8;
static const uint8_t step = 240/bw;
static const uint8_t gaps = 2;
static const uint8_t blHeight = 10;
static uint16_t color = 0xffff;
static const uint8_t offset = 10;

static uint8_t blocksCache[8] = {0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f,0x3f};
static int firstRender = 1;
static int score = 0;
static _Bool scoreUpdate = 1;
static int lastBat;
static _Bool reset = 0;
static int lives = 3;
    
void reset_breakout(struct Ball* ball) {
    ball->x = 120;
    ball->y = 120;
    ball->vx = 1;
    ball->vy = -2;
    for (int i = 0; i < 8; i++) blocksCache[i] = 0x3f;
    firstRender = 1;
    score = 0;
    scoreUpdate = 1;
    reset = 0;

    drawSquare(0, 0, 239, 239, 0x0000);
    for (int row = 0; row < 6; row++) {
        if (row < 2)
            color = 0xf800;
        else if (row < 4)
            color = 0xfc40;
        else 
            color = 0x47E0;

        for (int column = 0; column < bw; column++) {
            uint8_t x1 = column*step+gaps;
            uint8_t y1 = offset + blHeight*row + gaps;
            uint8_t x2 = step+column*step-gaps;
            uint8_t y2 = offset + blHeight*(row+1) - gaps;

            drawSquare(x1, y1, x2, y2, color);

        }
    }
}

void render_breakout (uint8_t bat, struct Ball* ball) {

    if (score == 176)
        reset = 1;

    if (reset) {
        reset_breakout(ball);
    }
    


    // bats can't cross walls
    int batWidth = 40;
    int batHeight = 7;
    if (bat > 239 - batWidth / 2)
        bat = 239 - batWidth / 2;
    if (bat < batWidth / 2)
        bat = batWidth / 2;



    uint8_t batX1 = bat - batWidth/2;
    uint8_t batY1 = 239 - batHeight-1;
    uint8_t batX2 = bat + batWidth/2-1;
    uint8_t batY2 = 239 - 2;

    // draw bat
    drawSquare(batX1, batY1, batX2, batY2, 0x555f); 


    if (bat != lastBat) {
        bool sideToRemoveFrom = bat < lastBat;

        uint8_t lastBatX1 = lastBat - batWidth/2;
        uint8_t lastBatX2 = lastBat + batWidth/2-1;

        // if the bat moved to the right
        if (!sideToRemoveFrom) {
            drawSquare(lastBatX1, batY1, batX1-1, batY2, 0x0000); 
        } else {
            drawSquare(batX2+1, batY1, lastBatX2, batY2, 0x0000); 
        }

    }

    lastBat = bat;


    
    int ballW = 6;
    int ballH = 4;

    // collision with blocks
    for (int row = 0; row < 6; row++) {
        for (int column = 0; column < bw; column++) {
            if ((blocksCache[column] >> row) & 1) {
                uint8_t x1 = column*step + gaps;
                uint8_t y1 = offset + blHeight*row + gaps;
                uint8_t x2 = step+column*step-gaps;
                uint8_t y2 = offset + blHeight*(row+1) - gaps;

                uint8_t ball_x1 = ball->x + ball->vx;
                uint8_t ball_y1 = ball->y + ball->vy;
                uint8_t ball_x2 = ball_x1 + ballW;
                uint8_t ball_y2 = ball_y1 + ballH;


                int destroy = 0;
                if (ball_x2 >= x1 && ball_x2 <= x2 && ball_y1 >= y1 && ball_y1 <= y2)
                    destroy = 1;

                if (ball_x2 >= x1 && ball_x2 <= x2 && ball_y2 >= y1 && ball_y2 <= y2)
                    destroy = 1;

                if (ball_x1 >= x1 && ball_x1 <= x2 && ball_y1 >= y1 && ball_y1 <= y2)
                    destroy = 1;

                if (ball_x1 >= x1 && ball_x1 <= x2 && ball_y2 >= y1 && ball_y2 <= y2)
                    destroy = 1;


                if (destroy) {
                    if (row < 2)
                        score += 7;
                    else if (row < 4)
                        score += 3;
                    else 
                        score ++;

                    scoreUpdate = 1;
                    drawSquare(x1, y1, x2, y2, 0x0000);
                    blocksCache[column] &= ~(1 << row);

                    int ballCenterX = ball_x1 + ballW/2;
                    int ballCenterY = ball_y1 + ballH/2;
                    // TODO: make bounce direction detection less shitty
                    if (ballCenterY > y1 && ballCenterY < y2) {
                        ball->vx = -ball->vx;
                    } else {
                        ball->vy = -ball->vy;
                    }
                }
            }
        }
    }


    uint8_t ball_x1 = ball->x + ball->vx;
    uint8_t ball_y1 = ball->y + ball->vy;
    uint8_t ball_x2 = ball_x1 + ballW;
    uint8_t ball_y2 = ball_y1 + ballH;


    int ballCenterX = ball_x1 + ballW/2;
    // on bat collision with ball
    if (ball_x2 >= batX1 && ball_x2 <= batX2 && ball_y2 >= batY1 && ball_y2 <= batY2) {
        // if hit in center
        if (abs(ballCenterX - bat) < (batWidth / 4)) {
            ball->vy = -ball->vy;
        } 
        // if hit on right side
        else if (ballCenterX > bat) {
            ball->vy = -ball->vy;
            if (ball->vx == 1)
                ball->vx = 2;
            if (ball->vx < 0)
                ball->vx = 1;
        }
        // if hit on left side
        else if (ballCenterX < bat) {
            ball->vy = -ball->vy;
            if (ball->vx == -1)
                ball->vx = -2;
            if (ball->vx > 0)
                ball->vx = -1;
        }
    }


    // collision with outside screen
    if (ball->x + ball->vx < 0)
        ball->vx = -ball->vx;

    if (ball->y + ball->vy < 0)
        ball->vy = -ball->vy;

    if (ball->x + ballW-1 + ball->vx > 239)
        ball->vx = -ball->vx;

    int dead = 0;
    if (ball->y + ballH-1 + ball->vy > 239) {
        dead = 1;
        lives--;
        drawSquare(ball->x, ball->y, ball->x+ballW-1, ball->y+ballH-1, 0x0000);
        ball->x = 120;
        ball->y = 120;
        if (lives == 0) {
            reset = 1;
            lives = 3;
        }
        //ball->vy = -ball->vy;
    }



    uint8_t x1;
    uint8_t y1;
    uint8_t width;
    uint8_t height;

    // area to remove from the side of the bal
    width = abs(ball->vx);

    if (width > 0) {
        if (ball->vx >= 0) {
            x1 = ball->x;
        } else {
            x1 = ball->x + ballW-1 + ball->vx + 1;
        }

        y1 = ball->y;
        height = ballH;
        drawSquare(x1, y1, x1 + width-1, y1 + height-1, 0x0000); 
    }

    // area to remove from the top/bottom of the ball
    height = abs(ball->vy);

    if (height > 0) {
        if (ball->vy >= 0) {
            y1 = ball->y;
        } else {
            y1 = ball->y + ballH-1 + ball->vy + 1;
        }

        x1 = ball->x;
        width = ballW;
        drawSquare(x1, y1, x1 + width-1, y1 + height-1, 0x0000); 
    }


    // move ball
    ball->x += ball->vx;
    ball->y += ball->vy;


    // refresh score display when ball is intersecting it
    if ((ball->x + ballW) > 213 && ball->y < (offset+gaps-1))
        scoreUpdate = 1;

    if (scoreUpdate) {
        drawSquare(206, 0, 237, offset+gaps-1, 0x0000);
        drawNumber(230, -1, score, 0xffff, 0x0000, 0, 1);
        scoreUpdate = 0;
    }


    // draw next ball
    if (!dead) 
        drawSquare(ball->x, ball->y, ball->x+ballW-1, ball->y+ballH-1, 0xffff);
    else 
        drawSquare(ball->x, ball->y, ball->x+ballW-1, ball->y+ballH-1, 0x0000);
}

struct Ball ball;
struct touchPoints touchPoint;
int batLocation;

void breakout_init() {
    ball.x = 120;
    ball.y = 120;
    ball.vx = -1;
    ball.vy = 1;

    breakout.trigger = create_time_event(70);

    batLocation = 0;

    NRF_GPIOTE->CONFIG[3] = GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos |
        2 << GPIOTE_CONFIG_POLARITY_Pos |
        13 << GPIOTE_CONFIG_PSEL_Pos;

    reset_breakout(&ball);
}

void breakout_run() {
    touch_refresh(&touchPoint);

    if (touchPoint.event)
        if (touchPoint.touchX != 0)
            if (touchPoint.touchX > 120)
                batLocation += 4;
            else 
                batLocation -= 4;

    if (batLocation > 239) batLocation = 239;
    if (batLocation < 0) batLocation = 0;

    render_breakout(batLocation , &ball);
    
    if (NRF_GPIOTE->EVENTS_IN[3]) {
        NRF_GPIOTE->EVENTS_IN[3] = 0;
        system_task(stop, &breakout);
        system_task(start, &main_menu);
    }
}

void breakout_stop() {
    drawSquare(0, 0, 239, 239, 0x0000);
}
