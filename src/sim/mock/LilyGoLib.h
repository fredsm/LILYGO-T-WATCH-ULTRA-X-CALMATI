#pragma once
#include <lvgl.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
inline uint32_t millis() { return lv_tick_get(); }
struct SimLilyGo { void vibrator() {} bool isCardReady() const { return true; } };
inline SimLilyGo instance;

struct SimSerial { template<class... Args> void printf(const char* fmt, Args... args) { std::printf(fmt,args...); } };
inline SimSerial Serial;
