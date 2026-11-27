#include "power_manager.h"
#include "power_hold.h"
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
#include <Wire.h>
#include <WiFi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include "usb_sd.h"
#include "mic_rec.h"
#include "wake_word.h"
#include "wardriver_screen.h"
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
bool valid_cpu_frequency(uint32_t mhz) {
    return mhz == 80 || mhz == 160 || mhz == 240;
}
void load_cpu_preference() {
    uint32_t saved = CPU_DEFAULT_MHZ;
    Preferences prefs;
    if (prefs.begin("watchpower", true)) {
        saved = prefs.getUInt("cpu_mhz", CPU_DEFAULT_MHZ);
        prefs.end();
    }
    preferred_cpu_mhz = valid_cpu_frequency(saved) ? saved : CPU_DEFAULT_MHZ;
    if (!restoreCpuFrequency()) {
        preferred_cpu_mhz = CPU_DEFAULT_MHZ;
        restoreCpuFrequency();
    }
    cpu_initialized = true;
}
uint32_t last_activity = 0, screenoff_timeout = 60000;
bool consume_click = false;
bool locked = false, off = false, asleep = false, requested = false;
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
#ifdef TWATCH_SIMULATOR
    return sim::hardware.watch.cpu_frequency();
#else
    return getCpuFrequencyMhz();
#endif
}
uint32_t getPreferredCpuFrequency() { return preferred_cpu_mhz; }

bool increaseCpuFrequency() {
    const uint32_t current = getCpuFrequency();
    return setCpuFrequency(current < 80 ? 80 : current < 160 ? 160 : 240);
}
bool decreaseCpuFrequency() {
    const uint32_t current = getCpuFrequency();
    return setCpuFrequency(current > 240 ? 240 : current > 160 ? 160 : 80);
}
bool restoreCpuFrequency() { return setCpuFrequency(preferred_cpu_mhz); }

bool setCpuFrequencyPreference(uint32_t mhz) {
    if (!valid_cpu_frequency(mhz)) return false;
    const uint32_t previous = getCpuFrequency();
    if (!setCpuFrequency(mhz)) {
        if (valid_cpu_frequency(previous)) setCpuFrequency(previous);
        return false;
    }
    if (mhz == preferred_cpu_mhz) return true;
    // CPU policy is device configuration, independent of removable SD storage.
    Preferences prefs;
    const bool opened = prefs.begin("watchpower", false);
    const bool saved = opened && prefs.putUInt("cpu_mhz", mhz) == sizeof(uint32_t);
    if (opened) prefs.end();
    if (!saved) {
        if (valid_cpu_frequency(previous)) setCpuFrequency(previous);
        return false;
    }
    preferred_cpu_mhz = mhz;
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
    instance.setBrightness(0);
    // No further LVGL flushes while the OLED is sleeping. Other timers may
    // still run during orderly shutdown (e.g. recording finalization).
    auto* timer = lv_display_get_refr_timer(lv_display_get_default());
    if (timer) lv_timer_pause(timer);
    instance.sleepDisplay();
#endif
}

void screenon() {
    if (!off || power_shutting_down()) return;
    off = false;
    sync_input();
#ifdef TWATCH_SIMULATOR
    sim::hardware.watch.set_display_on(true);
#else
    instance.wakeupDisplay();
    auto* timer = lv_display_get_refr_timer(lv_display_get_default());
    if (timer) lv_timer_resume(timer);
    lv_obj_invalidate(lv_screen_active());
#endif
    instance.setBrightness(settings_get_brightness());
    power_note_activity(millis());
}

void power_init() {
    // Resuming light sleep must not overwrite a temporary runtime override.
    if (!cpu_initialized) load_cpu_preference();
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
    screenlock();
#ifdef TWATCH_SIMULATOR
    screenoff();
    light_asleep = true;
#else
    pin_lock_on_screenoff();
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    instance.lightSleep(WAKEUP_SRC_POWER_KEY);
    power_init();
    // The SDK already woke the panel; restore a previously paused refresh.
    if (off) {
        off = false;
        auto* timer = lv_display_get_refr_timer(lv_display_get_default());
        if (timer) lv_timer_resume(timer);
    }
    instance.setBrightness(settings_get_brightness());
    lv_obj_invalidate(lv_screen_active());
    screenunlock();
    power_note_activity(millis());
    consume_click = true;
    pin_lock_handle_button();
#endif
}

void poweroff() { deepsleep(); }

void deepsleep() {
    if (asleep) return;
    screenlock();
    screenoff();
    asleep = true;
#ifdef TWATCH_SIMULATOR
    sim::leave_screen();
#else
    // Finish local SD writers before LilyGoLib unmounts and powers off SD.
    // USB hosts should eject first; unsynced host writes cannot be recovered.
    usb_sd_stop();
    wardriver_stop();
    wake_word_stop();
    mic_rec_stop();
    const uint32_t started = millis();
    while (mic_rec_is_recording() && uint32_t(millis() - started) < 2000) {
        lv_timer_handler();
        delay(10);
    }
    WiFi.mode(WIFI_OFF);
    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) esp_bluedroid_disable();
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) esp_bt_controller_disable();
    // Explicitly remove any previous wake sources; SDK handles PMU IRQ,
    // panel, touch, peripheral rails, SD/SPI/I2C and ESP deep sleep.
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    deep_wake_cookie = deep_wake_magic;
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
        load_cpu_preference();
        pin_lock_restore();
        sim::show_home();
        sim::start_boot(false);
    } else {
        pin_lock_handle_button();
    }
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
    if (hold.update(now)) { screenlock(); screenoff(); }
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
