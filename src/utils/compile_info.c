#include <string.h>
#include <stdio.h>
#include "calctime.h"

 unsigned long getCompileTime() {
    datetime time;
    sscanf(__TIME__, "%d:%d:%d", &time.hour,&time.minute,&time.second);

    char monthStr[4];
    sscanf(__DATE__, "%s %d %d", monthStr, &time.day, &time.year);

    time.month = 0;
    char *months[] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    while (strcmp(monthStr, months[time.month]))
        time.month++;
    time.month++;

    return timetoepoch(time);
}
