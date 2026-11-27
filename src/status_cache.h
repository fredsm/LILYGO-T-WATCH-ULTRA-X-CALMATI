#pragma once
#include <time.h>
// RTC/PMU have one hardware owner (UI). Consumers
// read cached values instead of each performing their own I2C transaction.
void status_cache_sample_clock(); // force read after clock changes
void status_cache_poll_clock();   // periodic refresh; cache advances between reads
void status_cache_sample_battery();
void status_cache_clock(struct tm* utc);
int status_cache_battery_percent();
bool status_cache_charging();
