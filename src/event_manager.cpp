#include "event_manager.h"
#ifndef TWATCH_SIMULATOR
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <mutex>
#endif
namespace {
constexpr unsigned kinds=static_cast<unsigned>(UiEventKind::Count);
UiEvent slots[kinds];
bool pending[kinds]={};
EventStats counters;
#ifdef TWATCH_SIMULATOR
std::mutex mux;
using Guard=std::lock_guard<std::mutex>;
#else
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { explicit Guard(portMUX_TYPE& m):m_(m){portENTER_CRITICAL(&m_);} ~Guard(){portEXIT_CRITICAL(&m_);} portMUX_TYPE& m_; };
TaskHandle_t owner=nullptr;
#endif
}
void event_manager_init(){
    Guard guard(mux);
    for(unsigned i=0;i<kinds;++i)pending[i]=false;
    counters=EventStats{};
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
#endif
}
bool ui_task_is_owner(){
#ifdef TWATCH_SIMULATOR
    return true;
#else
    return !owner || owner==xTaskGetCurrentTaskHandle();
#endif
}
