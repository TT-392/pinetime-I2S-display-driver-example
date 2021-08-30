#pragma once

typedef struct {
    int second;
    int minute;
    int hour;
    int day;
    int month;
    int year;
} datetime;

unsigned long timetoepoch(datetime time);

datetime epochtotime(unsigned long timeSinceEpoch);

unsigned long addTime(unsigned int epochTime, datetime timedelta);

