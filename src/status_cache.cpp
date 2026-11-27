#include "status_cache.h"
#include "event_manager.h"
#include "i2c_bus_manager.h"
#include <LilyGoLib.h>
#ifndef TWATCH_SIMULATOR
#include <freertos/FreeRTOS.h>
#endif
namespace {
struct tm base={};
uint32_t base_at=0;
bool clock_valid=false,battery_valid=false,charging=false;
int battery=0;
#ifdef TWATCH_SIMULATOR
struct Guard {};
#else
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard {Guard(){portENTER_CRITICAL(&mux);}~Guard(){portEXIT_CRITICAL(&mux);}};
#endif
}
void status_cache_sample_clock(){
    if(!ui_task_is_owner())return;
    if(!i2c_bus_ready(0x51))return;
    const auto before=i2c_bus_failures();
    UiEvent e;e.kind=UiEventKind::Clock;instance.rtc.getDateTime(&e.utc);
    if(i2c_bus_failures()!=before)return;
    e.generation=millis();event_publish(e);
    if(event_take(UiEventKind::Clock,e)){
        Guard guard;base=e.utc;base_at=e.generation;clock_valid=true;
    }
}
void status_cache_poll_clock(){
    bool refresh;
    {Guard guard;refresh=!clock_valid || uint32_t(millis()-base_at)>=30000;}
    if(refresh)status_cache_sample_clock();
}
void status_cache_sample_battery(){
    if(!ui_task_is_owner())return;
    if(!i2c_bus_ready(0x34))return;
    const auto before=i2c_bus_failures();
    UiEvent e;e.kind=UiEventKind::Battery;e.value=instance.pmu.getBatteryPercent();e.first=instance.pmu.isCharging();
    if(i2c_bus_failures()!=before)return;
    event_publish(e);
    if(event_take(UiEventKind::Battery,e)){
        Guard guard;battery=e.value;charging=e.first;battery_valid=true;
    }
}
void status_cache_clock(struct tm* utc){
    bool valid;
    {Guard guard;valid=clock_valid;}
    if(!valid && ui_task_is_owner())status_cache_sample_clock();
    uint32_t at;
    {Guard guard;*utc=base;at=base_at;valid=clock_valid;}
    if(valid){utc->tm_sec+=uint32_t(millis()-at)/1000;mktime(utc);}
}
int status_cache_battery_percent(){
    bool valid;
    {Guard guard;valid=battery_valid;}
    if(!valid && ui_task_is_owner())status_cache_sample_battery();
    Guard guard;return battery;
}
bool status_cache_charging(){
    bool valid;
    {Guard guard;valid=battery_valid;}
    if(!valid && ui_task_is_owner())status_cache_sample_battery();
    Guard guard;return charging;
}
