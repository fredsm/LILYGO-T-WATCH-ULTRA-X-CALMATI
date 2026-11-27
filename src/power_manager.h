#pragma once
#include <stdint.h>

// Reusable power primitives. Screen lock is an input lock, not authentication.
void screenlock();
void screenunlock();
void screenoff();
void screenon();
void lightsleep(); // reserved for idle policy; only call when peripherals are idle
void poweroff();   // user-facing full power-off action
void deepsleep();  // full power-off; only power key is a configured wake source
bool power_screen_locked();
bool power_screen_off();
bool power_is_asleep();
bool power_shutting_down();
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
