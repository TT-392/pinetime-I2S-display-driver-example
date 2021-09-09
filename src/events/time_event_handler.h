#pragma once
#include "system.h"

extern process time_event_handler;

volatile bool *create_time_event(int frequency);

void free_time_event(volatile bool *flag);
