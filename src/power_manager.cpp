#include "power_manager.h"
#include "power_hold.h"
#include "cpu_governor.h"
#include "cpu_load_sampler.h"
#include "diagnostics_manager.h"
#include "gps_service.h"
#include "gps_screen.h"
#include "pin_lock_screen.h"
#include <LilyGoLib.h>
#include <lvgl.h>
#include <Preferences.h>
#ifndef TWATCH_SIMULATOR
#include <esp_sleep.h>
#include <esp32-hal-cpu.h>
#include <esp_attr.h>
#include <esp_idf_version.h>
#include <esp_timer.h>
#include <esp_freertos_hooks.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>
#include <Wire.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include "usb_sd.h"
#include "storage_manager.h"
#include "mic_rec.h"
#include "wake_word.h"
#include "wardriver_screen.h"
#include "tpms.h"
#include "mifare.h"
#include "i2c_bus_manager.h"
#include "qspi_display.h"
#include "radio_manager.h"
#include "audio_manager.h"
#include "adsb.h"
#include "alarm.h"
#include "settings_screen.h"
#else
#include "sim/screens.hpp"
#include "sim/startup.hpp"
#include "settings_screen.h"
#endif

namespace {
PowerHold hold;
uint32_t preferred_cpu_mhz = CPU_DEFAULT_MHZ;
bool cpu_initialized = false;
CpuGovernor governor;
bool governor_running = false;
int cpu_load[2] = {-1, -1};

#ifndef TWATCH_SIMULATOR
// Frequency changes from the UI and governor worker must never overlap.
StaticSemaphore_t cpu_mutex_storage;
SemaphoreHandle_t cpu_mutex = nullptr;
struct CpuGuard {
    CpuGuard() { if (cpu_mutex) xSemaphoreTakeRecursive(cpu_mutex, portMAX_DELAY); }
    ~CpuGuard() { if (cpu_mutex) xSemaphoreGiveRecursive(cpu_mutex); }
};
#else
struct CpuGuard {};
#endif

bool valid_cpu_frequency(uint32_t mhz) {
    return mhz == 80 || mhz == 160 || mhz == 240;
}
bool valid_cpu_preference(uint32_t mhz) {
    return mhz == CPU_AUTO || valid_cpu_frequency(mhz);
}
void governor_sample(unsigned core0, unsigned core1, uint32_t elapsed_ms = CpuGovernor::window_ms) {
    CpuGuard guard;
    if (!governor_running || preferred_cpu_mhz != CPU_AUTO) return;
    cpu_load[0] = core0;
    cpu_load[1] = core1;
    const uint32_t current = getCpuFrequency();
    const uint32_t wanted = governor.evaluate(current, core0, core1, elapsed_ms);
    if (wanted != current && !setCpuFrequency(wanted)) governor.reset();
}

#ifdef TWATCH_SIMULATOR
lv_timer_t* governor_timer = nullptr;
bool prepare_governor() {
    if (governor_timer) return true;
    governor_timer = lv_timer_create([](lv_timer_t*) {
        governor_sample(sim::hardware.core_load[0], sim::hardware.core_load[1]);
    }, CpuGovernor::window_ms, nullptr);
    if (!governor_timer) return false;
    lv_timer_pause(governor_timer);
    return true;
}
bool start_governor_timer() {
    lv_timer_reset(governor_timer);
    lv_timer_resume(governor_timer);
    return true;
}
void stop_governor_timer() { if (governor_timer) lv_timer_pause(governor_timer); }
#else
// The SDK has PM_ENABLE and runtime stats disabled. Reuse the
// lightweight per-core scheduler samples collected for diagnostics.
TaskHandle_t governor_task = nullptr;
esp_timer_handle_t governor_timer = nullptr;
uint32_t previous_total[2] = {}, previous_busy[2] = {};
int64_t last_sample_us = 0;
void snapshot(unsigned core, uint32_t& total, uint32_t& busy) {
    const auto sample = cpu_sampler_snapshot(core);
    total = sample.total;
    busy = sample.busy;
}
void governor_worker(void*) {
    for (;;) {
        // No polling: an esp_timer callback releases this blocked task.
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        CpuGuard guard;
        if (!governor_running || preferred_cpu_mhz != CPU_AUTO) continue;
        const int64_t now = esp_timer_get_time();
        const int64_t elapsed_us = now - last_sample_us;
        last_sample_us = now;
        unsigned load[2] = {};
        bool valid = elapsed_us >= 400000 && elapsed_us <= 1000000;
        for (unsigned core = 0; core < 2; ++core) {
            uint32_t total, busy;
            snapshot(core, total, busy);
            const uint32_t samples = total - previous_total[core];
            const uint32_t active = busy - previous_busy[core];
            previous_total[core] = total;
            previous_busy[core] = busy;
            // Skip incomplete or stale windows, e.g. across a sleep boundary.
            const uint32_t expected = valid ? uint32_t(elapsed_us * configTICK_RATE_HZ / 1000000) : 0;
            if (!expected || samples < expected * 8 / 10 || samples > expected * 12 / 10 || active > samples) {
                valid = false;
            } else {
                // Round up so 80.1% qualifies as >80%, and exactly 80% does not.
                load[core] = (active * 100u + samples - 1) / samples;
            }
        }
        if (valid) governor_sample(load[0], load[1], uint32_t(elapsed_us / 1000));
        else {
            governor.reset();
            cpu_load[0] = cpu_load[1] = -1;
        }
    }
}
bool prepare_governor() {
    if (governor_timer) return true;
    if (!cpu_sampler_begin()) return false;
    esp_timer_create_args_t args = {};
    args.callback = [](void*) { if (governor_task) xTaskNotifyGive(governor_task); };
    args.name = "cpu_governor";
    if (esp_timer_create(&args, &governor_timer) == ESP_OK &&
        xTaskCreatePinnedToCore(governor_worker, "cpu_governor", 3072,
                               nullptr, tskIDLE_PRIORITY + 8, &governor_task, 1) == pdPASS)
        return true;
    if (governor_timer) { esp_timer_delete(governor_timer); governor_timer = nullptr; }
    return false;
}
bool start_governor_timer() {
    for (unsigned core = 0; core < 2; ++core)
        snapshot(core, previous_total[core], previous_busy[core]);
    last_sample_us = esp_timer_get_time();
    if (esp_timer_start_periodic(governor_timer, CpuGovernor::window_ms * 1000ULL) == ESP_OK)
        return true;
    return false;
}
void stop_governor_timer() {
    if (governor_timer) esp_timer_stop(governor_timer);
}
#endif

void stop_governor() {
    governor_running = false;
    stop_governor_timer();
    governor.reset();
    cpu_load[0] = cpu_load[1] = -1;
}
bool start_governor() {
    if (governor_running) return true;
    governor.reset();
    cpu_load[0] = cpu_load[1] = -1;
    if (!prepare_governor() || !start_governor_timer()) return false;
    governor_running = true;
    return true;
}
void load_cpu_preference() {
    CpuGuard guard;
    stop_governor();
    uint32_t saved = CPU_DEFAULT_MHZ;
    Preferences prefs;
    if (prefs.begin("watchpower", true)) {
        saved = prefs.getUInt("cpu_mhz", CPU_DEFAULT_MHZ);
        prefs.end();
    }
    preferred_cpu_mhz = valid_cpu_preference(saved) ? saved : CPU_DEFAULT_MHZ;
    if (!restoreCpuFrequency() || (preferred_cpu_mhz == CPU_AUTO && !start_governor())) {
        stop_governor();
        preferred_cpu_mhz = CPU_DEFAULT_MHZ;
        restoreCpuFrequency();
    }
    cpu_initialized = true;
}
void pause_cpu_governor() { CpuGuard guard; stop_governor(); }
void resume_cpu_governor() {
    CpuGuard guard;
    if (preferred_cpu_mhz == CPU_AUTO) {
        // Wake into a safe high-performance state until a fresh window arrives.
        setCpuFrequency(CPU_DEFAULT_MHZ);
        start_governor();
    }
}
uint32_t last_activity = 0, screenoff_timeout = 60000;
bool consume_click = false;
bool locked = false, off = false, asleep = false, requested = false;
#ifndef TWATCH_SIMULATOR
bool panel_transition_pending=false;
uint32_t panel_last_attempt=0;
void panel_transition(bool force=false) {
    if(!panel_transition_pending || !qspi_display_idle())return;
    const auto now=millis();
    if(!force && uint32_t(now-panel_last_attempt)<100)return;
    panel_last_attempt=now;
    if(off){
        instance.setBrightness(0);
        if(!qspi_display_idle() || !qspi_display_prepare_sleep())return;
        instance.sleepDisplay(); // SLPIN is already acknowledged; then gate panel rail
    }else{
        instance.wakeupDisplay();
        if(!qspi_display_panel_awake())return;
        instance.setBrightness(settings_get_brightness());
        if(!qspi_display_idle() || instance.getBrightness()!=settings_get_brightness())return;
        qspi_display_pause(false);
        auto* timer=lv_display_get_refr_timer(lv_display_get_default());
        if(timer)lv_timer_resume(timer);
        lv_obj_invalidate(lv_screen_active());
    }
    panel_transition_pending=false;
}
#endif

#ifdef TWATCH_SIMULATOR
bool light_asleep = false;
PowerHold wake_hold;
#else
// Only our intentional deep-sleep path sets this cookie. Cold boots remain normal.
RTC_DATA_ATTR uint32_t deep_wake_cookie = 0;
constexpr uint32_t deep_wake_magic = 0x50575235;
#endif
void sync_input() {
#ifndef TWATCH_SIMULATOR
    for (auto* input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
        if (!locked && !off) lv_indev_wait_release(input);
        lv_indev_enable(input, !locked && !off);
    }
#endif
}
}
bool setCpuFrequency(uint32_t mhz) {
    CpuGuard guard;
    if (!valid_cpu_frequency(mhz)) return false;
#ifdef TWATCH_SIMULATOR
    return sim::hardware.watch.set_cpu_frequency(mhz);
#else
    // The Arduino HAL handles clock configuration and peripheral callbacks.
    // Read the actual frequency back instead of trusting the request.
    return setCpuFrequencyMhz(mhz) && getCpuFrequencyMhz() == mhz;
#endif
}

uint32_t getCpuFrequency() {
    CpuGuard guard;
#ifdef TWATCH_SIMULATOR
    return sim::hardware.watch.cpu_frequency();
#else
    return getCpuFrequencyMhz();
#endif
}
uint32_t getPreferredCpuFrequency() { CpuGuard guard; return preferred_cpu_mhz; }
bool cpuAutomaticEnabled() { CpuGuard guard; return preferred_cpu_mhz == CPU_AUTO; }
int getCpuLoad(unsigned core) { CpuGuard guard; return core < 2 ? cpu_load[core] : -1; }

bool increaseCpuFrequency() {
    CpuGuard guard;
    const uint32_t current = getCpuFrequency();
    return setCpuFrequency(current < 80 ? 80 : current < 160 ? 160 : 240);
}
bool decreaseCpuFrequency() {
    CpuGuard guard;
    const uint32_t current = getCpuFrequency();
    return setCpuFrequency(current > 240 ? 240 : current > 160 ? 160 : 80);
}
bool restoreCpuFrequency() {
    CpuGuard guard;
    if (preferred_cpu_mhz == CPU_AUTO) {
        governor.reset();
        return setCpuFrequency(CPU_DEFAULT_MHZ);
    }
    return setCpuFrequency(preferred_cpu_mhz);
}

bool setCpuFrequencyPreference(uint32_t mhz) {
    CpuGuard guard;
    if (!valid_cpu_preference(mhz)) return false;
    const uint32_t previous = getCpuFrequency();
    const uint32_t old_preference = preferred_cpu_mhz;
    if (mhz == CPU_AUTO && old_preference == CPU_AUTO && governor_running) return true;
    if (!setCpuFrequency(mhz == CPU_AUTO ? CPU_DEFAULT_MHZ : mhz)) {
        if (valid_cpu_frequency(previous)) setCpuFrequency(previous);
        return false;
    }
    if (mhz == CPU_AUTO && !start_governor()) {
        if (valid_cpu_frequency(previous)) setCpuFrequency(previous);
        return false;
    }
    if (mhz != old_preference) {
        Preferences prefs;
        const bool opened = prefs.begin("watchpower", false);
        const bool saved = opened && prefs.putUInt("cpu_mhz", mhz) == sizeof(uint32_t);
        if (opened) prefs.end();
        if (!saved) {
            if (old_preference != CPU_AUTO) stop_governor();
            if (valid_cpu_frequency(previous)) setCpuFrequency(previous);
            return false;
        }
    }
    preferred_cpu_mhz = mhz;
    if (mhz != CPU_AUTO) stop_governor();
    return true;
}

void power_note_activity(uint32_t now) { last_activity = now; }
void power_set_screenoff_timeout(uint32_t ms, uint32_t now) {
    screenoff_timeout = ms;
    power_note_activity(now);
}
uint32_t power_get_screenoff_timeout() { return screenoff_timeout; }

bool power_screen_locked() { return locked; }
bool power_screen_off() { return off; }
bool power_is_asleep() {
#ifdef TWATCH_SIMULATOR
    return asleep || light_asleep;
#else
    return asleep;
#endif
}
bool power_shutting_down() { return hold.armed() || requested || asleep; }

void screenlock() {
    if (locked) return;
    locked = true;
    sync_input();
}

void screenunlock() {
    if (!locked || power_shutting_down()) return;
    locked = false;
    sync_input();
}

void screenoff() {
    if (off) return;
    off = true;
    sync_input();
    pin_lock_on_screenoff();
#ifdef TWATCH_SIMULATOR
    sim::hardware.watch.set_display_on(false);
#else
    // Pause first: no new transfers can start while transitioning the panel.
    qspi_display_pause(true);
    auto* timer = lv_display_get_refr_timer(lv_display_get_default());
    if (timer) lv_timer_pause(timer);
    panel_transition_pending=true;
    qspi_display_drain(); // at most one bounded wait; retry later if quarantined
    panel_transition(true);
#endif
}

void screenon() {
    if (!off || power_shutting_down()) return;
    off = false;
    sync_input();
#ifdef TWATCH_SIMULATOR
    sim::hardware.watch.set_display_on(true);
#else
    panel_transition_pending=true;
    qspi_display_drain();
    panel_transition(true);
#endif
#ifdef TWATCH_SIMULATOR
    instance.setBrightness(settings_get_brightness());
#endif
    power_note_activity(millis());
}

void power_init() {
    // Resuming light sleep must not overwrite a temporary runtime override.
    if (!cpu_initialized) {
#ifndef TWATCH_SIMULATOR
        cpu_mutex = xSemaphoreCreateRecursiveMutexStatic(&cpu_mutex_storage);
#endif
        load_cpu_preference();
    }
    power_note_activity(millis());
#ifndef TWATCH_SIMULATOR
    // AXP2101 generates independent falling/press and rising/release IRQs.
    // Keep a longer hardware emergency cutoff; the normal 5-second action
    // is software deep sleep, not the PMIC cutting all power rails.
    instance.pmu.setPowerKeyPressOffTime(XPOWERS_POWEROFF_10S);
    instance.pmu.enableIRQ(XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ);
#endif
}

void lightsleep() {
    // Do not invoke from the idle timer until peripheral ownership/resume
    // policy is implemented. The vendor routine shuts down several rails.
    if (power_shutting_down()) return;
#ifndef TWATCH_SIMULATOR
    if(radio_job_busy() || !audio_idle())return;
    audio_suspend(true);
    if(!audio_idle()){audio_suspend(false);return;}
#endif
    diagnostics_pause();
    gps_service_session(false);
    pause_cpu_governor();
    screenlock();
#ifdef TWATCH_SIMULATOR
    screenoff();
    light_asleep = true;
#else
    // Light sleep must not cut SD rails while the card is in use.
    storage_release_image_cache();
    storage_suspend();
    if(!storage_idle()){
        storage_resume();
        audio_suspend(false);
        gps_service_session(gps_screen_is_powered());
        resume_cpu_governor();
        screenunlock();
        diagnostics_resume();
        return;
    }
    qspi_display_pause(true);
    auto* refresh=lv_display_get_refr_timer(lv_display_get_default());
    if(refresh)lv_timer_pause(refresh);
    if(!qspi_display_drain() || !qspi_display_prepare_sleep()){
        // Do not cut a rail or enter vendor sleep while DMA owns a descriptor.
        panel_transition_pending=true;
        storage_resume();
        audio_suspend(false);
        gps_service_session(gps_screen_is_powered());
        resume_cpu_governor();
        screenunlock();
        diagnostics_resume();
        return;
    }
    alarm_stop_chime_loop();
    tpms_stop();
    mifare_dict_stop();
    pin_lock_on_screenoff();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    i2c_bus_runtime(false); // vendor lifecycle operations must not be skipped
    instance.lightSleep(WAKEUP_SRC_POWER_KEY);
    i2c_bus_runtime(true);
    if(instance.isCardReady())storage_media_inserted();
    else storage_media_removed();
    power_init();
    // SDK attempted panel wake; don't resume LVGL until it is acknowledged.
    off=false;
    panel_transition_pending=true;
    panel_transition(true);
    screenunlock();
    power_note_activity(millis());
    consume_click = true;
    pin_lock_handle_button();
    resume_cpu_governor();
    gps_service_session(gps_screen_is_powered());
    storage_resume();
    audio_suspend(false);
    diagnostics_resume();
#endif
}

void poweroff() { deepsleep(); }

void deepsleep() {
    if (asleep) return;
    diagnostics_pause();
    gps_service_session(false);
    pause_cpu_governor();
    screenlock();
    screenoff();
#ifndef TWATCH_SIMULATOR
    if(panel_transition_pending || !qspi_display_idle()){
        requested=true; // power_tick retries instead of freeing active DMA
        return;
    }
    radio_manager_suspend(true);
    audio_suspend(true);
    adsb_stop();
    wake_word_stop();
    mic_rec_stop();
    alarm_stop_chime_loop();
    alarm_service_audio();
    if(radio_job_busy() || !audio_idle()){
        requested=true; // let the workers release radio/I2S/SD resources first
        return;
    }
#endif
    asleep = true;
#ifdef TWATCH_SIMULATOR
    sim::leave_screen();
#else
    // Finish local SD writers before LilyGoLib unmounts and powers off SD.
    // USB hosts should eject first; unsynced host writes cannot be recovered.
    usb_sd_stop();
    wardriver_stop();
    // If the queue was full at stop, retry the final snapshot after accepted
    // jobs finish. Allocation/media failure is not retried indefinitely.
    if(wardriver_storage_pending() && storage_stats().worker && storage_stats().media){
        while(storage_stats().pending || storage_stats().active)delay(10);
        wardriver_bg_tick();
    }
    alarm_stop_chime_loop();
    tpms_stop();
    mifare_dict_stop();
    // Drain accepted immutable work before the SDK powers down SPI/SD.
    // Do not force an unmount during a hung in-flight operation.
    storage_release_image_cache();
    storage_suspend();
    while (!storage_idle()) {
        delay(10);
    }
    WiFi.mode(WIFI_OFF);
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) esp_bluedroid_disable();
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) esp_bt_controller_disable();
    // Explicitly remove any previous wake sources; SDK handles PMU IRQ,
    // panel, touch, peripheral rails, SD/SPI/I2C and ESP deep sleep.
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    deep_wake_cookie = deep_wake_magic;
    i2c_bus_runtime(false);
    instance.sleep(WAKEUP_SRC_POWER_KEY, false); // patched SDK arms key edges, retain RTC backup
    // A valid power-key wake configuration must never return.
    ESP.restart();
#endif
}

#ifdef TWATCH_SIMULATOR
namespace {
void sim_wake(uint32_t now) {
    const bool cold_boot = asleep;
    hold.reset(); wake_hold.reset(); requested = asleep = light_asleep = false;
    screenon();
    screenunlock();
    power_note_activity(now);
    consume_click = true;
    if (cold_boot) {
        diagnostics_reset();
        load_cpu_preference();
        pin_lock_restore();
        sim::show_home();
        sim::start_boot(false);
    } else {
        resume_cpu_governor();
        pin_lock_handle_button();
    }
    diagnostics_resume();
}
}
#endif

void power_button_down(uint32_t now) {
#ifdef TWATCH_SIMULATOR
    if (asleep) { consume_click = true; wake_hold.press(now); return; }
    if (light_asleep) { sim_wake(now); return; }
#endif
    if (requested || hold.armed()) return;
    power_note_activity(now);
    if (off) {
        consume_click = true;
        screenon();
        pin_lock_handle_button();
        power_note_activity(now);
        return;
    }
    consume_click = false;
    hold.press(now);
    consume_click = pin_lock_handle_button();
}
void power_button_up(uint32_t now) {
#ifdef TWATCH_SIMULATOR
    if (asleep) {
        if (wake_hold.release(now)) sim_wake(now);
        else wake_hold.reset();
        return;
    }
#endif
    power_note_activity(now);
    // A PIN-locked press is consumed for navigation, but still supports
    // the normal five-second shutdown hold. Wake presses have no active hold.
    if (hold.release(now)) requested = true;
}
bool power_allow_click() {
    if (consume_click) { consume_click = false; return false; }
    return !power_shutting_down() && !off && !power_is_asleep() && !pin_lock_active();
}
void power_tick(uint32_t now, bool check_idle) {
#ifdef TWATCH_SIMULATOR
    if (asleep) { if (wake_hold.update(now)) sim_wake(now); return; }
#endif
    if (power_is_asleep()) return;
#ifndef TWATCH_SIMULATOR
    panel_transition();
#endif
    if (hold.update(now)) { diagnostics_pause(); pause_cpu_governor(); screenlock(); screenoff(); }
    if (requested) { poweroff(); return; }
    if (check_idle && !off && !hold.down() && screenoff_timeout &&
        uint32_t(now - last_activity) >= screenoff_timeout) screenoff();
}
void power_cancel_press() {
#ifdef TWATCH_SIMULATOR
    if (asleep) { wake_hold.reset(); return; }
#endif
    if (hold.armed()) requested = true;
    else hold.reset();
    consume_click = false;
}

#ifndef TWATCH_SIMULATOR
namespace {
// The PMIC has no native five-second long-press wake (its long IRQ tops out
// at 2.5 s). Wake the MCU on the falling edge, but leave display/rails asleep
// and qualify the hold BEFORE normal SDK startup. A rejected tap re-arms
// deep sleep without initializing the display, SD or radios.
constexpr uint8_t key_down_mask = uint8_t(XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ >> 8);
constexpr uint8_t key_up_mask = uint8_t(XPOWERS_AXP2101_PKEY_POSITIVE_IRQ >> 8);
int key_status() {
    Wire.beginTransmission(AXP2101_SLAVE_ADDRESS);
    Wire.write(uint8_t(XPOWERS_AXP2101_INTSTS2));
    if (Wire.endTransmission(false) != 0) return -1;
    if (Wire.requestFrom(uint8_t(AXP2101_SLAVE_ADDRESS), uint8_t(1)) != 1) return -1;
    return Wire.read();
}
bool clear_key_status(uint8_t mask) {
    Wire.beginTransmission(AXP2101_SLAVE_ADDRESS);
    Wire.write(uint8_t(XPOWERS_AXP2101_INTSTS2));
    Wire.write(mask);
    return Wire.endTransmission() == 0;
}
void reject_wake(uint8_t observed) {
    // W1C only observed edges: a newly arriving press must remain pending.
    if (!clear_key_status(observed & (key_down_mask | key_up_mask))) return;
    Wire.end();
    pinMode(SDA, OPEN_DRAIN);
    pinMode(SCL, OPEN_DRAIN);
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
#if ESP_IDF_VERSION >= ESP_IDF_VERSION_VAL(5,0,0)
    esp_sleep_enable_ext1_wakeup_io(1ULL << PMU_INT, ESP_EXT1_WAKEUP_ANY_LOW);
#else
    esp_sleep_enable_ext1_wakeup(1ULL << PMU_INT, ESP_EXT1_WAKEUP_ANY_LOW);
#endif
    esp_deep_sleep_start();
}
}
void power_boot_gate() {
    if (deep_wake_cookie != deep_wake_magic ||
        esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_EXT1) {
        deep_wake_cookie = 0;
        return;
    }
    Wire.begin(SDA, SCL);
    Wire.setTimeOut(50);
    int status = key_status();
    if (status >= 0) {
        // Both edges already latched means the press ended before boot.
        if (!(status & key_down_mask) || (status & key_up_mask)) {
            reject_wake(uint8_t(status));
        } else {
            // Measure from MCU wake (startup latency is included as far as
            // the ESP timer permits). Never clear a release before sampling.
            while (esp_timer_get_time() < int64_t(PowerHold::hold_ms) * 1000) {
                status = key_status();
                if (status < 0) break; // recover normally if PMIC I2C fails
                if (status & key_up_mask) { reject_wake(uint8_t(status)); break; }
                delay(10);
            }
        }
    }
    // An I2C fault must not permanently trap the watch in an off state.
    deep_wake_cookie = 0;
    consume_click = true;
    Wire.end();
}
#else
void power_boot_gate() {}
#endif
