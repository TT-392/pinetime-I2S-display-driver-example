#pragma once

void set_system_timeout(int timeoutInTenthsOfSeconds);

extern process power_manager;

int power_manager_get_current_count();

int power_manager_get_current_timeout();

int power_manager_get_current_dummyCount();

