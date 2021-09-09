#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <nrf.h>
#include "system.h"

void time_event_handler_run ();
void time_event_handler_init ();

static task tasks[] = {{&time_event_handler_init, start, 0}, {&time_event_handler_run, run, 0}};

process time_event_handler = {
    .taskCnt = 2,
    .tasks = tasks,
    .trigger = &event_always
};

void time_event_handler_init () {
	NRF_CLOCK->LFCLKSRC = CLOCK_LFCLKSRC_SRC_Xtal << CLOCK_LFCLKSRC_SRC_Pos;
	NRF_CLOCK->EVENTS_LFCLKSTARTED = 0;
	NRF_CLOCK->TASKS_LFCLKSTART = 1;
	while(NRF_CLOCK->EVENTS_LFCLKSTARTED == 0);

	NRF_RTC1->PRESCALER = 0;
	NRF_RTC1->TASKS_START = 1;
    // overflows every 512 seconds, this is an integer, so all integer frequencies should be fine
}

typedef struct timeEvent {
    volatile bool flag;
    int frequency;
    uint64_t count;
    struct timeEvent *next;
} timeEvent_t;

static timeEvent_t *eventList = NULL;
volatile bool *create_time_event (int frequency) {
    timeEvent_t **current = &eventList;

    while (*current != NULL)
        current = &((*current)->next);

    (*current) = malloc(sizeof(timeEvent_t));
    (*current)->frequency = frequency;
    (*current)->flag = 0;
    (*current)->count = 0;
    (*current)->next = NULL;

    return &((*current)->flag);
}

void free_time_event(volatile bool *flag) {
    assert(eventList != NULL);

    if (&(eventList->flag) == flag) {
        timeEvent_t *temp = eventList;
        eventList = temp->next;
        free(temp);
        return;
    }

    timeEvent_t *previous = eventList;

    while (previous->next->next != NULL && &(previous->next->flag) != flag)
        previous = previous->next;

    if (previous->next->next == NULL && &(previous->next->flag) != flag)
        assert(0); // flag not in array

    timeEvent_t *current = previous->next;
    previous->next = current->next;
    free(current);
}

void time_event_handler_run () {
    timeEvent_t *current = eventList;

    while (current != NULL) {
        if ((NRF_RTC1->COUNTER * current->frequency) / 32768 != current->count) {
            current->flag = 1;
            current->count = (NRF_RTC1->COUNTER * current->frequency) / 32768;
        }
        current = current->next;
    }
}
