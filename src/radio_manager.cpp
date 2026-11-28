#include "radio_manager.h"
#ifdef TWATCH_SIMULATOR
#include <chrono>
#include <mutex>
#include <thread>
#else
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <esp_timer.h>
#include <esp_bt.h>
#endif

namespace {
#ifdef TWATCH_SIMULATOR
std::recursive_timed_mutex transition;
std::mutex metadata;
struct Guard { std::lock_guard<std::mutex> lock{metadata}; };
using Owner = std::thread::id;
Owner current(){return std::this_thread::get_id();}
uint64_t now(){
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}
bool take(){return transition.try_lock_for(std::chrono::milliseconds(20));}
void give(){transition.unlock();}
bool ble_live(){return false;}
#else
StaticSemaphore_t transition_memory;
SemaphoreHandle_t transition=nullptr;
portMUX_TYPE metadata=portMUX_INITIALIZER_UNLOCKED;
struct Guard {Guard(){portENTER_CRITICAL(&metadata);}~Guard(){portEXIT_CRITICAL(&metadata);}};
using Owner = TaskHandle_t;
Owner current(){return xTaskGetCurrentTaskHandle();}
uint64_t now(){return esp_timer_get_time();}
bool take(){return transition && xSemaphoreTakeRecursive(transition,pdMS_TO_TICKS(20))==pdTRUE;}
void give(){xSemaphoreGiveRecursive(transition);}
bool ble_live(){return esp_bt_controller_get_status()!=ESP_BT_CONTROLLER_STATUS_IDLE;}
#endif
RadioStats stats;
Owner owner{};
uint64_t job_started=0;
}
void radio_manager_init(){
#ifndef TWATCH_SIMULATOR
    if(!transition)transition=xSemaphoreCreateRecursiveMutexStatic(&transition_memory);
    configASSERT(transition);
#endif
}
bool radio_transition_enter(){
    if(take())return true;
    Guard g;++stats.lock_timeouts;return false;
}
void radio_transition_leave(){give();}
bool radio_action_allowed(RadioAction action,bool wifi,bool ble){
    Guard g;
    const bool wifi_action=action==RadioAction::WifiUp || action==RadioAction::WifiChange || action==RadioAction::WifiDown;
    const bool job_active=stats.job!=RadioJob::None;
    const bool own=job_active && owner==current();
    bool allowed=true;
    if(wifi_action && job_active && !own)allowed=false;
    if(action==RadioAction::WifiUp && ble)allowed=false;
    if(action==RadioAction::BleUp && (wifi || job_active))allowed=false;
    if(stats.suspended && !own && action!=RadioAction::WifiDown && action!=RadioAction::BleDown)allowed=false;
    if(!allowed)++stats.rejected;
    return allowed;
}
void radio_action_record(RadioAction action,bool success,uint64_t us){
    Guard g;
    bool wifi=action==RadioAction::WifiUp || action==RadioAction::WifiDown || action==RadioAction::WifiChange;
    auto& calls=wifi?stats.wifi_calls:stats.ble_calls;
    auto& maximum=wifi?stats.wifi_max_us:stats.ble_max_us;
    ++calls;
    if(!success)++stats.failures;
    if(us>maximum)maximum=us>UINT32_MAX?UINT32_MAX:uint32_t(us);
}
bool radio_job_begin(RadioJob job){
    RadioTransition lock;
    if(!lock)return false;
    bool bt=ble_live();
    Guard g;
    if(job==RadioJob::None || stats.suspended || stats.job!=RadioJob::None || bt){
        ++stats.rejected;return false;
    }
    stats.job=job;owner=current();job_started=now();return true;
}
void radio_job_end(RadioJob job){
    Guard g;
    if(stats.job==job && job!=RadioJob::None && owner==current()){
        stats.job=RadioJob::None;owner=Owner{};++stats.completed;
    }
}
bool radio_job_busy(){Guard g;return stats.job!=RadioJob::None;}
void radio_manager_suspend(bool suspend){Guard g;stats.suspended=suspend;}
RadioStats radio_manager_stats(){
    Guard g;
    auto s=stats;
    if(s.job!=RadioJob::None)s.job_ms=uint32_t((now()-job_started)/1000);
    return s;
}
const char* radio_job_name(RadioJob job){
    switch(job){
        case RadioJob::Weather:return "Weather";
        case RadioJob::TimeSync:return "Time sync";
        case RadioJob::Adsb:return "Aircraft";
        case RadioJob::Voice:return "Voice";
        case RadioJob::Layout:return "Layout download";
        default:return "Idle";
    }
}
