#include "diagnostics_manager.h"
#include "cpu_load_sampler.h"
#include "event_manager.h"
#include <lvgl.h>
#include <stdio.h>
#include <string.h>
#ifdef TWATCH_SIMULATOR
#include "sim/mock/hardware.hpp"
#else
#include <Arduino.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <esp_heap_caps.h>
#include <esp32-hal-cpu.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace {
constexpr uint32_t stall_ms = 100;
constexpr unsigned stages = static_cast<unsigned>(DiagStage::Count);
const char* const names[] = {
    "Idle / scheduling", "PMU / sensor events", "Power manager", "Motion",
    "Wi-Fi / BLE switch", "Web service", "USB console", "Network results",
    "Button navigation", "GPS UART", "Meshtastic / SPI", "Detector logs / SD",
    "Phone BLE", "ESP-NOW", "LVGL handler", "QSPI display flush",
    "TPMS / SPI", "Pager / SPI", "APRS / SPI", "Scan results / SD",
    "NFC / SPI", "Touch / haptic", "RTC / I2C", "Alarm", "Battery / I2C",
    "SD detect / mount", "Status indicators", "Wardriver / SD", "Screenshot / SD",
    "Captive portal", "AirTag / SD", "Flipper / SD", "Skimmer / SD", "Evil Twin / SD",
    "Flock / SD", "Threat Radar / SD", "Handshake / SD", "NFC write / SPI", "NFC credit / SPI", "MIFARE / SPI"
};
static_assert(sizeof(names)/sizeof(names[0]) == stages, "stage names");
DiagSnapshot state;
uint32_t boot_ms = 0, last_ui = 0, stage_since = 0;
DiagToken flush_token{DiagStage::Idle, 0};
bool in_flush = false;
DiagStage longest_stage = DiagStage::Idle;
uint32_t longest_stage_ms = 0, stall_culprit_ms = 0;
#ifdef TWATCH_SIMULATOR
struct Guard {};
uint32_t now_ms() { return lv_tick_get(); }
#else
portMUX_TYPE state_mux = portMUX_INITIALIZER_UNLOCKED;
struct Guard {
    Guard() { portENTER_CRITICAL(&state_mux); }
    ~Guard() { portEXIT_CRITICAL(&state_mux); }
};
TaskHandle_t monitor = nullptr, ui_task = nullptr;
uint32_t now_ms() { return uint32_t(esp_timer_get_time() / 1000); }
#endif

void timing(DiagStage stage, uint32_t duration) {
    auto& t = state.timings[static_cast<unsigned>(stage)];
    t.last = duration;
    if (duration > t.maximum) t.maximum = duration;
    ++t.count;
    if (stage != DiagStage::Idle && duration > longest_stage_ms) {
        longest_stage = stage;
        longest_stage_ms = duration;
    }
}
void add_stall(uint32_t at, uint32_t gap, DiagStage stage) {
    for (unsigned i = DIAG_HISTORY - 1; i > 0; --i) state.history[i] = state.history[i-1];
    state.history[0].at=at; state.history[0].duration=gap;
    state.history[0].stage=stage; state.history[0].recovered=false;
    if (state.history_count < DIAG_HISTORY) ++state.history_count;
    ++state.stalls;
    state.stalled = true;
}
void observe(uint32_t now, bool completed) {
    if (!state.ready || state.paused) return;
    const uint32_t gap = now - last_ui;
    state.uptime_ms = now - boot_ms;
    state.ui_gap_ms = gap;
    state.active_ms = now - stage_since;
    if (gap > state.max_ui_gap_ms) state.max_ui_gap_ms = gap;
    if (gap >= stall_ms) {
        DiagStage culprit = state.active;
        uint32_t culprit_ms = now - stage_since;
        if (culprit == DiagStage::Idle || longest_stage_ms > culprit_ms) {
            culprit = longest_stage;
            culprit_ms = longest_stage_ms;
        }
        if (!state.stalled) {
            add_stall(last_ui - boot_ms, gap, culprit);
            stall_culprit_ms = culprit_ms;
        } else if (culprit != DiagStage::Idle && culprit_ms > stall_culprit_ms) {
            // Keep the more specific flush attribution if its enclosing LVGL
            // handler is only marginally longer than the observed transfer.
            if (!(state.history[0].stage == DiagStage::QspiFlush &&
                  culprit == DiagStage::LVGL && culprit_ms - stall_culprit_ms <= 20)) {
                state.history[0].stage = culprit;
                stall_culprit_ms = culprit_ms;
            }
        }
        state.history[0].duration = gap;
    }
    if (completed) {
        if (state.stalled) state.history[0].recovered = true;
        state.stalled = false;
        last_ui = now;
        state.ui_gap_ms = 0;
        longest_stage_ms = stall_culprit_ms = 0;
        longest_stage = DiagStage::Idle;
    }
}
void flush_event(lv_event_t* e) {
    if (lv_event_get_code(e) == LV_EVENT_FLUSH_START ||
        lv_event_get_code(e) == LV_EVENT_FLUSH_WAIT_START) {
        if (!in_flush) { flush_token = diagnostics_enter(DiagStage::QspiFlush); in_flush = true; }
    } else if (in_flush) {
        diagnostics_exit(flush_token);
        in_flush = false;
    }
}

#ifndef TWATCH_SIMULATOR
const char* reset_name() {
    switch (esp_reset_reason()) {
        case ESP_RST_POWERON: return "Power on";
        case ESP_RST_SW: return "Software reset";
        case ESP_RST_PANIC: return "Panic";
        case ESP_RST_INT_WDT: return "Interrupt watchdog";
        case ESP_RST_TASK_WDT: return "Task watchdog";
        case ESP_RST_WDT: return "Watchdog";
        case ESP_RST_BROWNOUT: return "Brownout";
        case ESP_RST_DEEPSLEEP: return "Deep sleep wake";
        default: return "Other";
    }
}
void report(const DiagStall& event, bool recovered) {
    char line[140];
    int n = snprintf(line, sizeof(line), "[diag] UI %s %lums | %s\n",
                     recovered ? "recovered" : "stalled",
                     (unsigned long)event.duration, diagnostics_stage_name(event.stage));
    // Best effort; USB CDC can wait despite availableForWrite(). This runs
    // only in the disposable telemetry task, never in the heartbeat monitor.
    if (n > 0 && n < int(sizeof(line)) && Serial.availableForWrite() >= n)
        Serial.write(reinterpret_cast<const uint8_t*>(line), n);
}
// The high-priority heartbeat monitor never prints, allocates heap or
// waits on peripheral/driver mutexes. A separate low-priority telemetry task
// performs potentially blocking SDK heap queries and USB output.
void monitor_task(void*) {
    CpuSamples prev[2] = {cpu_sampler_snapshot(0), cpu_sampler_snapshot(1)};
    uint32_t sample_at = now_ms();
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(50));
        uint32_t now = now_ms();
        bool paused, sampling;
        {
            Guard lock;
            observe(now_ms(), false);
            paused = state.paused;
            sampling = state.cpu_sampling;
        }
        if (now - sample_at >= 500) {
            uint32_t elapsed = now - sample_at;
            sample_at = now;
            int loads[2] = {-1,-1};
            for (unsigned i=0; i<2; ++i) {
                CpuSamples next = cpu_sampler_snapshot(i);
                uint32_t total = next.total-prev[i].total, busy = next.busy-prev[i].busy;
                prev[i]=next;
                uint32_t expected = elapsed * configTICK_RATE_HZ / 1000;
                if (sampling && !paused && elapsed <= 1000 && total &&
                    total >= expected*8/10 && total <= expected*12/10 && busy <= total)
                    loads[i] = int((busy*100u+total-1)/total);
            }
            uint32_t mhz=getCpuFrequencyMhz();
            Guard lock;
            state.core0=loads[0]; state.core1=loads[1];
            state.cpu_mhz=mhz;
        }
    }
}
void telemetry_task(void*) {
    uint32_t heap_at=now_ms(), logged=0, logged_duration=0;
    bool logged_recovery=false;
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(250));
        uint32_t now=now_ms();
        DiagStall event;
        bool emit=false, paused;
        {
            Guard lock;
            paused=state.paused;
            if (state.history_count) {
                event=state.history[0];
                if (state.stalls!=logged || (event.recovered && !logged_recovery) ||
                    (!event.recovered && event.duration - logged_duration >= 5000)) {
                    logged=state.stalls; logged_duration=event.duration;
                    logged_recovery=event.recovered; emit=true;
                }
            }
        }
        if (paused) continue;
        if (emit) report(event,event.recovered);
        if (now-heap_at>=1000) {
            heap_at=now;
            uint32_t heap=heap_caps_get_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
            uint32_t min_heap=heap_caps_get_minimum_free_size(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
            uint32_t largest=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
            uint32_t psram=heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
            uint32_t stack=ui_task ? uxTaskGetStackHighWaterMark(ui_task) : 0;
            Guard lock;
            state.heap=heap; state.min_heap=min_heap; state.largest=largest;
            state.psram=psram; state.ui_stack=stack;
            state.memory_sample_ms=now-boot_ms;
        }
    }
}
#else
void mock_metrics() {
    state.core0 = int(sim::hardware.core_load[0]); state.core1 = int(sim::hardware.core_load[1]);
    state.cpu_mhz = sim::hardware.watch.cpu_frequency();
    state.heap=108*1024; state.min_heap=92*1024; state.largest=78*1024;
    state.psram=6*1024*1024; state.ui_stack=4096;
    state.memory_sample_ms=now_ms()-boot_ms;
}
#endif
}

const char* diagnostics_stage_name(DiagStage stage) {
    unsigned i=static_cast<unsigned>(stage);
    return i<stages?names[i]:"Unknown";
}
void diagnostics_init() {
    if (state.ready) return;
    const uint32_t now=now_ms();
    boot_ms=last_ui=stage_since=now;
    state.ready=true;
#ifdef TWATCH_SIMULATOR
    state.simulated=true; state.cpu_sampling=true; state.reset_reason="Simulated boot";
    mock_metrics();
#else
    state.reset_reason=reset_name();
    state.cpu_sampling=cpu_sampler_begin();
    ui_task=xTaskGetCurrentTaskHandle();
    if (xTaskCreatePinnedToCore(monitor_task,"diagnostics",3072,nullptr,5,&monitor,0)!=pdPASS) {
        state.ready=false;
        return;
    }
    // If this allocation fails the heartbeat still runs; memory/USB stats
    // remain unavailable. No fatal error during boot.
    xTaskCreatePinnedToCore(telemetry_task,"diag_telemetry",3072,nullptr,1,nullptr,0);
#endif
    auto* display=lv_display_get_default();
    if (display) {
        lv_display_add_event_cb(display,flush_event,LV_EVENT_FLUSH_START,nullptr);
        lv_display_add_event_cb(display,flush_event,LV_EVENT_FLUSH_FINISH,nullptr);
        lv_display_add_event_cb(display,flush_event,LV_EVENT_FLUSH_WAIT_START,nullptr);
        lv_display_add_event_cb(display,flush_event,LV_EVENT_FLUSH_WAIT_FINISH,nullptr);
    }
}
DiagToken diagnostics_enter(DiagStage stage) {
    const uint32_t now=now_ms();
    Guard lock;
    DiagToken token{state.active,stage_since};
    if (state.ready) { state.active=stage; stage_since=now; }
    return token;
}
void diagnostics_exit(DiagToken token) {
    const uint32_t now=now_ms();
    Guard lock;
    if (state.ready && !state.paused) timing(state.active, now-stage_since);
    state.active=token.previous;
    stage_since=token.since;
}
void diagnostics_ui_handler() {
    if(!ui_task_is_owner())return;
    uint32_t start=now_ms();
    { DiagScope scope(DiagStage::LVGL); lv_timer_handler(); }
    uint32_t now=now_ms();
    Guard lock;
    if (!state.ready || state.paused) return;
    state.last_ui_ms=now-start;
    observe(now,true);
}
void diagnostics_pause() {
    Guard lock;
    state.paused=true;
    if (state.stalled) state.history[0].recovered=true;
    state.stalled=false; state.ui_gap_ms=0;
}
void diagnostics_resume() {
    uint32_t now=now_ms();
    Guard lock;
    state.paused=false; last_ui=stage_since=now;
    state.active=DiagStage::Idle; state.ui_gap_ms=0;
    longest_stage_ms=stall_culprit_ms=0; longest_stage=DiagStage::Idle;
}
void diagnostics_reset() {
    uint32_t now=now_ms();
    Guard lock;
    // Only used at mock reboot. Preserve initialized display callbacks.
    const bool ready=state.ready, sim=state.simulated, cpu=state.cpu_sampling;
    const char* reason=state.reset_reason;
    state=DiagSnapshot{};
    state.ready=ready; state.simulated=sim; state.cpu_sampling=cpu; state.reset_reason=reason;
    boot_ms=last_ui=stage_since=now;
    longest_stage_ms=stall_culprit_ms=0; longest_stage=DiagStage::Idle;
}
DiagSnapshot diagnostics_snapshot() {
    Guard lock;
    return state;
}
#ifdef TWATCH_SIMULATOR
void diagnostics_sim_tick() {
    Guard lock;
    mock_metrics();
    observe(now_ms(),false);
}
void diagnostics_simulate_stall(DiagStage stage, uint32_t duration) {
    if (!state.ready || state.paused || !duration || stage==DiagStage::Count) return;
    // Replay a synthetic stalled interval without freezing SDL or advancing the
    // watch's clock, screen-off timer, or PIN/power state.
    uint32_t now=now_ms();
    Guard lock;
    const auto prior=state.active; const uint32_t prior_since=stage_since;
    state.active=stage; stage_since=last_ui=now;
    observe(now+duration,false);
    timing(stage,duration);
    observe(now+duration,true);
    state.active=prior; stage_since=prior_since; last_ui=now;
    state.uptime_ms=now-boot_ms; state.ui_gap_ms=0; state.active_ms=0;
}
#endif
