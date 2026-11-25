#pragma once
#include <lvgl.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
inline uint32_t millis() { return lv_tick_get(); }
struct SimGpsValue {
 bool isValid() const {return true;} unsigned long age() const {return 0;}
 double lat() const {return 35.0;} double lng() const {return -80.0;}
 int year() const {return 2026;} int month() const {return 1;} int day() const {return 1;}
 int hour() const {return 12;} int minute() const {return 0;} int second() const {return 0;}
 unsigned long value() const {return 8;} double meters() const {return 220;} double kmph() const {return 0;} double deg() const {return 0;}
};
struct SimGps {SimGpsValue location,date,time,satellites,altitude,speed,course;};
struct SimRtc {template<class... T> void setDateTime(T...){} void hwClockRead(){}};
enum {POWER_GPS,POWER_RADIO};
struct SimRadio {void setRxBoostedGainMode(bool,bool){}}; inline SimRadio radio;
struct SimLilyGo { SimGps gps; SimRtc rtc; void powerControl(int,bool){} void initLoRa(){} void setBrightness(uint8_t){}  void vibrator() {} bool isCardReady() const { return true; } };
inline SimLilyGo instance;

struct SimSerial { template<class... Args> void printf(const char* fmt, Args... args) { std::printf(fmt,args...); } };
inline SimSerial Serial;

struct SimUart {void end(){}}; inline SimUart Serial1;

#include "Arduino.h"
