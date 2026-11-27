#include "power_lifecycle.h"
void PowerLifecycle::screen(bool off,uint32_t now){
    if(shutting_ || stats_.stage==PowerStage::LightSleep)return;
    stats_.stage=off?PowerStage::ScreenOff:PowerStage::Awake;
    stats_.blockers=0;
    if(off)off_at_=now;
}
bool PowerLifecycle::idle_due(uint32_t now) const{
    return stats_.stage==PowerStage::ScreenOff && uint32_t(now-off_at_)>=idle_grace_ms;
}
void PowerLifecycle::begin_shutdown(uint32_t now){
    if(shutting_)return;
    shutting_=true;shutdown_at_=now;stats_.stage=PowerStage::Stopping;stats_.blockers=0;
}
bool PowerLifecycle::advance(uint32_t blockers,uint32_t now){
    if(!shutting_ || stats_.stage==PowerStage::DeepSleep)return false;
    stats_.blockers=blockers;stats_.shutdown_blockers|=blockers;
    const auto elapsed=uint32_t(now-shutdown_at_);
    if(elapsed>stats_.max_drain_ms)stats_.max_drain_ms=elapsed;
    if(blockers)return false;
    switch(stats_.stage){
        case PowerStage::Stopping:stats_.stage=PowerStage::FinalStorage;break;
        case PowerStage::FinalStorage:stats_.stage=PowerStage::Draining;break;
        case PowerStage::Draining:stats_.stage=PowerStage::Drivers;break;
        case PowerStage::Drivers:stats_.stage=PowerStage::DeepSleep;stats_.shutdown_ms=elapsed;break;
        default:return false;
    }
    return true;
}
void PowerLifecycle::light_enter(uint32_t now){
    if(shutting_)return;
    stats_.stage=PowerStage::LightSleep;stats_.blockers=0;stats_.last_idle_blockers=0;light_at_=now;++stats_.light_entries;
}
void PowerLifecycle::light_exit(uint32_t now,bool slept){
    if(stats_.stage!=PowerStage::LightSleep)return;
    stats_.last_light_ms=uint32_t(now-light_at_);
    if(slept)++stats_.light_wakes;else ++stats_.light_aborts;
    stats_.stage=PowerStage::ScreenOff;off_at_=now;stats_.blockers=0;
}
void PowerLifecycle::reboot(){stats_=PowerLifecycleStats{};shutting_=false;off_at_=shutdown_at_=light_at_=0;}
PowerLifecycleStats PowerLifecycle::snapshot(uint32_t now) const{
    auto s=stats_;
    if(shutting_ && s.stage!=PowerStage::DeepSleep)s.shutdown_ms=uint32_t(now-shutdown_at_);
    return s;
}
const char* power_stage_name(PowerStage stage){
    switch(stage){
        case PowerStage::ScreenOff:return "Screen off";
        case PowerStage::LightSleep:return "Light sleep";
        case PowerStage::Stopping:return "Stopping workers";
        case PowerStage::FinalStorage:return "Final storage";
        case PowerStage::Draining:return "Draining";
        case PowerStage::Drivers:return "Driver shutdown";
        case PowerStage::DeepSleep:return "Deep sleep";
        default:return "Awake";
    }
}
