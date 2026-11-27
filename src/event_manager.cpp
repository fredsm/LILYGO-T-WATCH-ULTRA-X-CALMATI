#include "event_manager.h"
#ifndef TWATCH_SIMULATOR
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_attr.h>
#else
#include <mutex>
#include <thread>
#endif
namespace {
constexpr unsigned kinds=static_cast<unsigned>(UiEventKind::Count);
UiEvent slots[kinds];
bool pending[kinds]={};
EventStats counters;
bool back_pending=false;
#ifdef TWATCH_SIMULATOR
std::mutex mux;
using Guard=std::lock_guard<std::mutex>;
std::thread::id owner;
#else
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { explicit Guard(portMUX_TYPE& m):m_(m){portENTER_CRITICAL(&m_);} ~Guard(){portEXIT_CRITICAL(&m_);} portMUX_TYPE& m_; };
TaskHandle_t owner=nullptr;
#endif
}
void event_manager_init(){
    Guard guard(mux);
    for(unsigned i=0;i<kinds;++i)pending[i]=false;
    counters=EventStats{};back_pending=false;
}
bool event_publish(const UiEvent& event){
    unsigned i=static_cast<unsigned>(event.kind);
    if(i>=kinds)return false;
    Guard guard(mux);
    if(pending[i])++counters.replaced;else ++counters.pending;
    slots[i]=event;pending[i]=true;++counters.published;
    return true;
}
bool event_take(UiEventKind kind,UiEvent& event){
    unsigned i=static_cast<unsigned>(kind);
    if(i>=kinds)return false;
    Guard guard(mux);
    if(!pending[i])return false;
    event=slots[i];pending[i]=false;--counters.pending;++counters.consumed;
    return true;
}
EventStats event_stats(){Guard guard(mux);return counters;}
void ui_task_claim(){
#ifndef TWATCH_SIMULATOR
    owner=xTaskGetCurrentTaskHandle();
    // Keep the SDK priority until all legacy I/O has moved off this owner.
    // Raising the entire old loop would also raise its blocking operations.
#else
    owner=std::this_thread::get_id();
#endif
}
bool ui_task_is_owner(){
#ifdef TWATCH_SIMULATOR
    return owner==std::thread::id{} || owner==std::this_thread::get_id();
#else
    return !owner || owner==xTaskGetCurrentTaskHandle();
#endif
}

#ifdef TWATCH_SIMULATOR
void event_back_from_isr() {
    Guard guard(mux);
#else
void IRAM_ATTR event_back_from_isr() {
    portENTER_CRITICAL_ISR(&mux);
#endif
    if(back_pending) ++counters.back_coalesced;
    back_pending=true;
#ifndef TWATCH_SIMULATOR
    portEXIT_CRITICAL_ISR(&mux);
#endif
}
bool event_take_back() {
    Guard guard(mux);
    const bool pending=back_pending;
    back_pending=false;
    return pending;
}
