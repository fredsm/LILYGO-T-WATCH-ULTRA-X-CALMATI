#pragma once
#include <time.h>
// RTC/PMU still have one hardware owner (UI) until I2C Manager. Consumers
// read cached values instead of each performing their own I2C transaction.
void status_cache_sample_clock();
void status_cache_sample_battery();
void status_cache_clock(struct tm* utc);
int status_cache_battery_percent();
bool status_cache_charging();
