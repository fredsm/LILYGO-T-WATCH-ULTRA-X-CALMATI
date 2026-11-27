#include "i2c_bus_manager.h"
#include <assert.h>
#ifdef TWATCH_SIMULATOR
#include <chrono>
#include <mutex>
#else
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <esp_timer.h>
#endif

namespace {
constexpr uint32_t MAX_RUNTIME_MS=50;
constexpr uint32_t COOLDOWN_US=100000;
constexpr uint32_t CLIENT_RETRY_US=100000;
#ifdef TWATCH_SIMULATOR
std::timed_mutex bus;
std::mutex metadata;
struct Guard { std::lock_guard<std::mutex> lock{metadata}; };
uint64_t now_us(){
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}
bool take(){return bus.try_lock_for(std::chrono::milliseconds(MAX_RUNTIME_MS));}
void give(){bus.unlock();}
#else
StaticSemaphore_t bus_memory;
SemaphoreHandle_t bus=nullptr;
portMUX_TYPE metadata=portMUX_INITIALIZER_UNLOCKED;
struct Guard {Guard(){portENTER_CRITICAL(&metadata);}~Guard(){portEXIT_CRITICAL(&metadata);}};
uint64_t now_us(){return esp_timer_get_time();}
bool take(){configASSERT(bus);return xSemaphoreTake(bus,pdMS_TO_TICKS(MAX_RUNTIME_MS))==pdTRUE;}
void give(){xSemaphoreGive(bus);}
#endif
I2cBusStats stats;
bool runtime=false;
uint64_t started=0, retry_at=0;
uint64_t client_retry[static_cast<unsigned>(I2cClient::Count)]={};
unsigned idx(uint16_t a){return static_cast<unsigned>(i2c_client(a));}
uint32_t elapsed(uint64_t start){auto us=now_us()-start;return us>UINT32_MAX?UINT32_MAX:uint32_t(us);}
bool cooling(){return runtime && now_us()<retry_at;}
}
I2cClient i2c_client(uint16_t address){
    switch(address){
        case 0x34: return I2cClient::Pmu;
        case 0x1a: return I2cClient::Touch;
        case 0x5a: return I2cClient::TouchHaptic; // hardware variants use 0x5a for touch as well
        case 0x51: return I2cClient::Rtc;
        case 0x28: case 0x29: return I2cClient::Motion;
        case 0x20: return I2cClient::Expander;
        default: return I2cClient::Other;
    }
}
const char* i2c_client_name(I2cClient c){
    switch(c){
        case I2cClient::Pmu: return "PMU";
        case I2cClient::Touch: return "Touch";
        case I2cClient::TouchHaptic: return "Haptic/touch";
        case I2cClient::Rtc: return "RTC";
        case I2cClient::Motion: return "Motion";
        case I2cClient::Expander: return "SD/GPIO";
        default: return "Other";
    }
}
void i2c_bus_init(){
#ifndef TWATCH_SIMULATOR
    if(!bus)bus=xSemaphoreCreateMutexStatic(&bus_memory);
    configASSERT(bus);
#endif
}
void i2c_bus_runtime(bool enabled){
    Guard g;runtime=enabled;retry_at=0;
    for(auto& at:client_retry)at=0;
}
bool i2c_bus_ready(uint16_t address){
    Guard g;
    if(stats.active || cooling() || (runtime && now_us()<client_retry[idx(address)])){
        ++stats.clients[idx(address)].deferred;return false;
    }
    return true;
}
uint32_t i2c_bus_failures(){Guard g;return stats.failures;}
uint32_t i2c_bus_timeout(uint32_t requested_ms){
    Guard g;return runtime && requested_ms>MAX_RUNTIME_MS?MAX_RUNTIME_MS:requested_ms;
}
bool i2c_bus_acquire(uint16_t address){
    auto start=now_us();
    {
        Guard g;
        if(cooling()){++stats.clients[idx(address)].deferred;++stats.failures;return false;}
    }
    if(!take()){
        Guard g;++stats.admission_timeouts;++stats.failures;
        ++stats.clients[idx(address)].deferred;return false;
    }
    bool admitted;
    {
        Guard g;
        admitted=!cooling();
        auto& c=stats.clients[idx(address)];
        if(admitted){
            auto wait=elapsed(start);if(wait>c.max_wait_us)c.max_wait_us=wait;
            ++c.transactions;stats.active=true;stats.address=address;started=now_us();
        }else{++c.deferred;++stats.failures;}
    }
    if(!admitted)give();
    return admitted;
}
void i2c_bus_release(I2cResult result){
    {
        Guard g;assert(stats.active);
        auto i=idx(stats.address);auto& c=stats.clients[i];
        auto duration=elapsed(started);if(duration>c.max_transfer_us)c.max_transfer_us=duration;
        if(result==I2cResult::Ok){
            if(stats.recovering){++stats.recoveries;stats.recovering=false;}
        }else{
            ++stats.failures;
            if(runtime)client_retry[i]=now_us()+CLIENT_RETRY_US;
            if(result==I2cResult::Timeout){
                ++c.timeouts;stats.recovering=true;
                // ESP-IDF resets the controller and clears the bus on hardware timeout.
                // Do not retry immediately or manually toggle pins during a live transfer.
                if(runtime)retry_at=now_us()+COOLDOWN_US;
            }else if(result==I2cResult::Nack)++c.nacks;
            else ++c.errors;
        }
        stats.active=false;
    }
    give();
}
I2cBusStats i2c_bus_stats(){
    Guard g;auto s=stats;s.active_us=stats.active?elapsed(started):0;s.cooling=cooling();return s;
}
// C hooks used by the pinned SDK patch; no include-path coupling to vendor libs.
extern "C" bool twatch_i2c_ready(uint16_t address){return i2c_bus_ready(address);}
extern "C" uint32_t twatch_i2c_failures(){return i2c_bus_failures();}
