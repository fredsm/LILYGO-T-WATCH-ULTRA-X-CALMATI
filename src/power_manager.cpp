#include "power_manager.h"
#include "power_hold.h"
#include <LilyGoLib.h>
#include <lvgl.h>
#ifndef TWATCH_SIMULATOR
#include <esp_sleep.h>
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
bool locked = false, off = false, asleep = false, requested = false;
#ifdef TWATCH_SIMULATOR
bool light_asleep = false;
#endif
}

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
#ifndef TWATCH_SIMULATOR
    // Physical buttons are independent of LVGL input; touch cannot navigate.
    for (auto* input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input))
        lv_indev_enable(input, false);
#endif
}

void screenunlock() {
    if (!locked || power_shutting_down()) return;
    locked = false;
#ifndef TWATCH_SIMULATOR
    for (auto* input = lv_indev_get_next(nullptr); input; input = lv_indev_get_next(input)) {
        lv_indev_wait_release(input);
        lv_indev_enable(input, true);
    }
#endif
}

void screenoff() {
    if (off) return;
    off = true;
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
#ifdef TWATCH_SIMULATOR
    sim::hardware.watch.set_display_on(true);
#else
    instance.wakeupDisplay();
    auto* timer = lv_display_get_refr_timer(lv_display_get_default());
    if (timer) lv_timer_resume(timer);
    lv_obj_invalidate(lv_screen_active());
#endif
    instance.setBrightness(settings_get_brightness());
}

void power_init() {
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
    instance.sleep(WAKEUP_SRC_POWER_KEY, false); // retain RTC backup domain
    // A valid power-key wake configuration must never return.
    ESP.restart();
#endif
}

void power_button_down(uint32_t now) { if (!power_is_asleep() && !requested) hold.press(now); }
void power_button_up(uint32_t now) { if (!power_is_asleep() && hold.release(now)) requested = true; }
void power_tick(uint32_t now) {
    if (power_is_asleep()) return;
    if (hold.update(now)) { screenlock(); screenoff(); }
    // Wait for release before arming the wake IRQ; the original hold must
    // not immediately wake the freshly sleeping device.
    if (requested) poweroff();
}
void power_cancel_press() {
    // A host focus loss must not leave a phantom hold. Once armed, treat
    // loss of the contact as release, preserving the requested shutdown.
    if (hold.armed()) requested = true;
    else hold.reset();
}
#ifdef TWATCH_SIMULATOR
void power_sim_wake() {
    if (!power_is_asleep()) return;
    const bool cold_boot = asleep;
    hold.reset(); requested = asleep = light_asleep = false;
    screenon();
    screenunlock();
    if (cold_boot) {
        sim::show_home();
        sim::start_boot(false);
    }
}
#endif
