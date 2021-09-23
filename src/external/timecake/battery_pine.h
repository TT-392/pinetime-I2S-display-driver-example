#pragma once

int battery_setup(void);
void battery_read(int *flags,float *voltage,float *percent);
