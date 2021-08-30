#include "calctime.h"
int months[12] = {31,28,31,30,31,30,31,31,30,31,30,31};

_Bool leapyear(int y){
    return (((y%4 == 0) && (y % 100 != 0)) || ((y%4 == 0) && (y % 400 == 0)));
}

unsigned long timetoepoch(datetime time) {
    unsigned long epoch = 0;
    for(int i = 1970;i < time.year;i++){
        epoch += (365+leapyear(i))*86400;
    }
    for (int i=0;i < time.month-1;i++){
        epoch += months[i]*86400;
    }
    epoch += (time.month > 1)*leapyear(time.year)*86400;
    epoch += (time.day-1)*86400;
    epoch += time.hour*3600;
    epoch += time.minute*60;
    epoch += time.second;   
    return(epoch);
}

datetime epochtotime(unsigned long timeSinceEpoch) {
    datetime retval;
    unsigned long epoch = timeSinceEpoch;
    int i = 1970;
    while(epoch/((365+leapyear(i))*86400)){
        epoch -= (365+leapyear(i))*86400;       
        i++;
    }
    retval.year = i;
    i = 0;
    while(epoch/((months[i]*86400)+((i == 1)*leapyear(retval.year)*86400))){
        epoch -= ((months[i]*86400)+((i == 1)*leapyear(retval.year)*86400));
        i++;
    }

    retval.month = i+1;
    retval.day = epoch/86400+1;
    retval.hour = (epoch%86400)/3600;
    retval.minute = (epoch%3600)/60;
    retval.second = epoch%60;                
    return retval;
}

unsigned long addTime(unsigned int epochTime, datetime timedelta) {
    // TODO: this might mess up in some scenarios
    datetime time = epochtotime(epochTime);
    time.second += timedelta.second;
    time.minute += timedelta.minute;
    time.hour += timedelta.hour;
    time.day += timedelta.day;
    time.month += timedelta.month;
    time.year += timedelta.year;

    return timetoepoch (time);
}
