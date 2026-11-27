#include "cpu_load_sampler.h"
#ifndef TWATCH_SIMULATOR
#include <esp_attr.h>
#include <freertos/FreeRTOS.h>
#include <esp_freertos_hooks.h>
#include <freertos/task.h>

namespace {
DRAM_ATTR CpuSamples counters[2] = {};
DRAM_ATTR TaskHandle_t idle[2] = {};
DRAM_ATTR portMUX_TYPE mux[2] = {portMUX_INITIALIZER_UNLOCKED, portMUX_INITIALIZER_UNLOCKED};
bool initialized = false;
void IRAM_ATTR sample(unsigned core) {
    const bool busy = xTaskGetCurrentTaskHandleForCPU(core) != idle[core];
    portENTER_CRITICAL_ISR(&mux[core]);
    ++counters[core].total;
    if (busy) ++counters[core].busy;
    portEXIT_CRITICAL_ISR(&mux[core]);
}
void IRAM_ATTR sample0() { sample(0); }
void IRAM_ATTR sample1() { sample(1); }
}
bool cpu_sampler_begin() {
    // Called only from the UI/setup task.
    if (initialized) return true;
    idle[0] = xTaskGetIdleTaskHandleForCPU(0);
    idle[1] = xTaskGetIdleTaskHandleForCPU(1);
    if (!idle[0] || !idle[1]) return false;
    if (esp_register_freertos_tick_hook_for_cpu(sample0, 0) != ESP_OK) return false;
    if (esp_register_freertos_tick_hook_for_cpu(sample1, 1) != ESP_OK) {
        esp_deregister_freertos_tick_hook_for_cpu(sample0, 0);
        return false;
    }
    return initialized = true;
}
CpuSamples cpu_sampler_snapshot(unsigned core) {
    if (core >= 2) return {};
    portENTER_CRITICAL(&mux[core]);
    CpuSamples result = counters[core];
    portEXIT_CRITICAL(&mux[core]);
    return result;
}
#endif
