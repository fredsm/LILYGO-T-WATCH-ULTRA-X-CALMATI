#pragma once
#include <lvgl.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
inline uint32_t millis() { return lv_tick_get(); }
struct SimLilyGo { void vibrator() {} };
inline SimLilyGo instance;
