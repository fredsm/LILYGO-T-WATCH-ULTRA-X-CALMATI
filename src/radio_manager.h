#pragma once
#include <stdint.h>

enum class RadioJob : uint8_t { None, Weather, TimeSync, Adsb, Voice, Layout };
enum class RadioAction : uint8_t { WifiUp, WifiChange, WifiDown, BleUp, BleDown };
struct RadioStats {
    RadioJob job = RadioJob::None;
    bool suspended = false;
    uint32_t job_ms = 0, completed = 0, rejected = 0, lock_timeouts = 0;
    uint32_t wifi_calls = 0, ble_calls = 0, failures = 0;
    uint32_t wifi_max_us = 0, ble_max_us = 0;
};
void radio_manager_init();
bool radio_transition_enter();
void radio_transition_leave();
struct RadioTransition {
    RadioTransition(): acquired(radio_transition_enter()) {}
    ~RadioTransition(){if(acquired)radio_transition_leave();}
    RadioTransition(const RadioTransition&) = delete;
    RadioTransition& operator=(const RadioTransition&) = delete;
    explicit operator bool() const {return acquired;}
private:
    bool acquired;
};
// Called under the transition lock, using current driver state.
bool radio_action_allowed(RadioAction action, bool wifi_initialized, bool ble_initialized);
void radio_action_record(RadioAction action, bool success, uint64_t elapsed_us);
bool radio_job_begin(RadioJob job);
void radio_job_end(RadioJob job);
bool radio_job_busy();
void radio_manager_suspend(bool suspend);
RadioStats radio_manager_stats();
const char* radio_job_name(RadioJob job);
