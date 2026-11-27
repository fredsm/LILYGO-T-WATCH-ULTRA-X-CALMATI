#pragma once
#include <stdint.h>
#include <stddef.h>

// Main-loop operation boundaries, not raw bus transaction timings.
enum class DiagStage : uint8_t {
    Idle, HardwareEvents, Power, Motion, ScanRadio, Web, Console, Network,
    Buttons, GPS, Meshtastic, DetectorSD, Phone, EspNow, LVGL, QspiFlush,
    TPMS, Pager, APRS, ScanResultsSD, NFC, TouchHaptic, RTC, Alarm,
    Battery, SDDetect, Indicators, WardriverSD, ScreenshotSD, CaptivePortal, AirTagSD, FlipperSD, SkimmerSD, EvilTwinSD,
    FlockSD, ThreatRadarSD, HandshakeSD, NFCWrite, NFCCredit, MIFARE, Count
};
const char* diagnostics_stage_name(DiagStage stage);
struct DiagTiming { uint32_t last = 0, maximum = 0, count = 0; };
struct DiagStall { uint32_t at = 0, duration = 0; DiagStage stage = DiagStage::Idle; bool recovered = false; };
constexpr unsigned DIAG_HISTORY = 12;
struct DiagSnapshot {
    bool ready = false, paused = false, stalled = false, cpu_sampling = false;
    bool simulated = false;
    DiagStage active = DiagStage::Idle;
    uint32_t active_ms = 0, ui_gap_ms = 0, max_ui_gap_ms = 0, stalls = 0, uptime_ms = 0;
    uint32_t last_ui_ms = 0, memory_sample_ms = 0;
    uint32_t max_slice_ms = 0, slice_overruns = 0, slices = 0;
    int core0 = -1, core1 = -1;
    uint32_t cpu_mhz = 0, heap = 0, min_heap = 0, largest = 0, psram = 0, ui_stack = 0;
    const char* reset_reason = "Unknown";
    DiagTiming timings[static_cast<unsigned>(DiagStage::Count)] = {};
    DiagStall history[DIAG_HISTORY] = {};
    unsigned history_count = 0;
};
void diagnostics_init();
void diagnostics_pause();
void diagnostics_resume();
void diagnostics_reset();
DiagSnapshot diagnostics_snapshot();
struct DiagToken { DiagStage previous; uint32_t since; };
DiagToken diagnostics_enter(DiagStage stage);
void diagnostics_exit(DiagToken token);
void diagnostics_scheduler_slice(uint32_t elapsed_ms);
void diagnostics_ui_handler(); // replaces lv_task_handler at main-loop boundaries
#ifdef TWATCH_SIMULATOR
void diagnostics_sim_tick();
void diagnostics_simulate_stall(DiagStage stage, uint32_t duration);
#endif
class DiagScope {
    DiagToken previous_;
public:
    explicit DiagScope(DiagStage stage) : previous_(diagnostics_enter(stage)) {}
    ~DiagScope() { diagnostics_exit(previous_); }
    DiagScope(const DiagScope&) = delete;
    DiagScope& operator=(const DiagScope&) = delete;
};
#define DIAG_RUN(stage, call) do { DiagScope diagnostic_scope(DiagStage::stage); call; } while (0)
