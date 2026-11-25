#pragma once
// Native-only compatibility for UI files that only need Arduino millis().
#include <lvgl.h>
#include <cstdint>
#include <cstdio>
inline uint32_t millis() { return lv_tick_get(); }
