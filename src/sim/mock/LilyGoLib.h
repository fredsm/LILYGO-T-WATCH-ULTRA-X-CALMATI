#pragma once
#include <lvgl.h>
#include <cstdint>
#include <cstdio>
#include <cmath>
#include <ctime>
constexpr int DEVICE_MAX_BRIGHTNESS_LEVEL=255;
// A firmware busy-wait must advance even while the LVGL callback owns the thread.
// Keep a deterministic sub-tick clock; do not advance LVGL recursively.
inline void (*sim_micro_tick)(uint32_t)=nullptr;
inline uint32_t micros(){
 static uint64_t us=0;const uint64_t scheduled=uint64_t(lv_tick_get())*1000;
 us+=100;if(us<scheduled)us=scheduled;
 if(sim_micro_tick)sim_micro_tick(uint32_t(us));
 return uint32_t(us);
}
inline uint32_t millis() { return lv_tick_get(); }
struct SimGpsValue {
 bool isValid() const {return true;} unsigned long age() const {return 0;}
 double lat() const {return 35.0;} double lng() const {return -80.0;}
 int year() const {return 2026;} int month() const {return 1;} int day() const {return 1;}
 int hour() const {return 12;} int minute() const {return 0;} int second() const {return 0;}
 unsigned long value() const {return 8;} double meters() const {return 220;} double kmph() const {return 0;} double deg() const {return 0;}
};
struct SimGps {SimGpsValue location,date,time,satellites,altitude,speed,course;};
struct SimRtc {
 template<class... T> void setDateTime(T...){} void hwClockRead(){}
 void getDateTime(struct tm* out) const {
  // A fixed simulated epoch advances with LVGL, not the computer's clock.
  std::time_t value=1795608000 + lv_tick_get()/1000;
  *out=*std::gmtime(&value);
 }
};
enum {POWER_GPS,POWER_RADIO,POWER_NFC};
constexpr int RADIOLIB_ERR_NONE=0;
struct SimRadio {
 float frequency=915; bool receiving=false;
 void setRxBoostedGainMode(bool,bool){}
 template<class... T> int begin(float f,T...){frequency=f;return 0;}
 int setFrequency(float f,bool=false){frequency=f;return 0;}
 int startReceive(){receiving=true;return 0;}
 int standby(){receiving=false;return 0;}
 float getRSSI(bool=false){return receiving ? -110.0f + std::fmod(frequency*7+lv_tick_get()/100,65.0f) : -127;}
}; inline SimRadio radio;
struct SimPmu {int getBatteryPercent()const{return 75;} bool isCharging()const{return false;} bool nfc=false;bool isEnableDLDO1()const{return nfc;}};
struct SimLilyGo { SimPmu pmu; void initNFC(){}  SimGps gps; SimRtc rtc; void powerControl(int power,bool on){if(power==POWER_NFC)pmu.nfc=on;} void initLoRa(){} void setBrightness(uint8_t){}  void vibrator() {} bool isCardReady() const { return true; } };
inline SimLilyGo instance;

struct SimSerial { void println(const char* s){std::puts(s);} void print(const char* s){std::printf("%s",s);}  template<class... Args> void printf(const char* fmt, Args... args) { std::printf(fmt,args...); } };
inline SimSerial Serial;

struct SimUart {void end(){}}; inline SimUart Serial1;

#include "Arduino.h"

#include "nfc.hpp"

struct SimEsp {unsigned getFreeHeap()const{return 16*1024*1024;}};inline SimEsp ESP;
