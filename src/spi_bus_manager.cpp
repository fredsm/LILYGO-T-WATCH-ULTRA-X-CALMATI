#include "spi_bus_manager.h"
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
#ifdef TWATCH_SIMULATOR
std::recursive_mutex bus;
std::mutex metadata;
struct Guard { std::lock_guard<std::mutex> lock{metadata}; };
uint64_t now_us() {
    using namespace std::chrono;
    return duration_cast<microseconds>(steady_clock::now().time_since_epoch()).count();
}
bool try_lock() { return bus.try_lock(); }
void lock() { bus.lock(); }
void unlock() { bus.unlock(); }
#else
StaticSemaphore_t bus_memory;
SemaphoreHandle_t bus=nullptr;
portMUX_TYPE metadata=portMUX_INITIALIZER_UNLOCKED;
struct Guard {
    Guard() { portENTER_CRITICAL(&metadata); }
    ~Guard() { portEXIT_CRITICAL(&metadata); }
};
uint64_t now_us() { return esp_timer_get_time(); }
bool try_lock() { configASSERT(bus); return xSemaphoreTakeRecursive(bus,0)==pdTRUE; }
void lock() { xSemaphoreTakeRecursive(bus,portMAX_DELAY); }
void unlock() { xSemaphoreGiveRecursive(bus); }
#endif
SpiBusStats stats;
uint64_t acquired_at=0;
uint32_t elapsed(uint64_t start) {
    const uint64_t us=now_us()-start;
    return us>UINT32_MAX ? UINT32_MAX : uint32_t(us);
}
unsigned index(SpiClient c) { return static_cast<unsigned>(c); }
}
void spi_bus_init() {
#ifndef TWATCH_SIMULATOR
    if(!bus) bus=xSemaphoreCreateRecursiveMutexStatic(&bus_memory);
    configASSERT(bus);
#endif
}
void spi_bus_acquire(SpiClient client) {
    assert(index(client)<index(SpiClient::Count));
    const auto start=now_us();
    // Count pending entrants before trying the lock, including the small
    // interval between mutex acquisition and publishing ownership.
    { Guard g; ++stats.waiters; }
    const bool contended=!try_lock();
    if(contended) lock();
    {
        Guard g;
        --stats.waiters;
        auto& s=stats.clients[index(client)];
        ++s.transactions;
        if(contended) ++s.contended;
        const auto wait=elapsed(start);
        if(wait>s.max_wait_us) s.max_wait_us=wait;
        stats.owner=client;
        acquired_at=now_us();
        stats.active=true;
    }
}
void spi_bus_release() {
    {
        Guard g;
        assert(stats.active);
        auto& s=stats.clients[index(stats.owner)];
        const auto held=elapsed(acquired_at);
        if(held>s.max_hold_us) s.max_hold_us=held;
        stats.active=false;
    }
    unlock();
}
bool spi_bus_ui_ready(SpiClient client) {
    Guard g;
    if(stats.active || stats.waiters || stats.lifecycle) {
        ++stats.clients[index(client)].deferred;
        return false;
    }
    return true;
}
bool spi_bus_lifecycle_begin(){
    if(!try_lock())return false;
    bool ready;
    {Guard g;ready=!stats.active && !stats.waiters && !stats.lifecycle;if(ready)stats.lifecycle=true;}
    if(!ready)unlock();
    return ready;
}
void spi_bus_lifecycle_end(){
    {Guard g;assert(stats.lifecycle && !stats.active);stats.lifecycle=false;}
    unlock();
}
SpiBusStats spi_bus_stats() {
    Guard g;
    auto snapshot=stats;
    snapshot.active_us=stats.active?elapsed(acquired_at):0;
    return snapshot;
}
const char* spi_bus_client_name(SpiClient client) {
    switch(client) {
        case SpiClient::Sd: return "SD";
        case SpiClient::LoRa: return "LoRa";
        case SpiClient::Nfc: return "NFC";
        default: return "?";
    }
}
