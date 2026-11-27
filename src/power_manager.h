#pragma once
#include <stdint.h>
#include "power_lifecycle.h"

// Reusable power primitives. Screen lock is an input lock, not authentication.
void screenlock();
void screenunlock();
void screenoff();
void screenon();
void lightsleep(); // retentive standby, only when peripherals and deadlines permit
void poweroff();   // user-facing full power-off action
void deepsleep();  // full power-off; only power key is a configured wake source
bool power_screen_locked();
bool power_screen_off();
bool power_is_asleep();
bool power_shutting_down();
PowerLifecycleStats power_lifecycle_stats();
void power_init();
void power_boot_gate(); // early firmware boot: qualify deep-sleep wake before UI init
void power_note_activity(uint32_t now);
void power_set_screenoff_timeout(uint32_t ms, uint32_t now);
uint32_t power_get_screenoff_timeout();
bool power_allow_click(); // consume the wake press instead of navigating
void power_button_down(uint32_t now);
void power_button_up(uint32_t now);
void power_tick(uint32_t now, bool check_idle = true);
void power_cancel_press();

// CPU policy. Zero denotes Auto; runtime changes never persist the preference.
constexpr uint32_t CPU_DEFAULT_MHZ = 240;
constexpr uint32_t CPU_AUTO = 0;
bool setCpuFrequency(uint32_t mhz);
uint32_t getCpuFrequency();
bool increaseCpuFrequency(); // one step, saturates at 240 MHz
bool decreaseCpuFrequency(); // one step, saturates at 80 MHz
bool restoreCpuFrequency();  // restore the user's saved preference
bool setCpuFrequencyPreference(uint32_t mhz); // CPU_AUTO or a fixed rate
uint32_t getPreferredCpuFrequency();
bool cpuAutomaticEnabled();
// Last sampled utilization (0..100), or -1 when sampling is not active.
int getCpuLoad(unsigned core);
