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
void power_button_down(uint32_t now);
void power_button_up(uint32_t now);
void power_tick(uint32_t now);
void power_cancel_press();
#ifdef TWATCH_SIMULATOR
void power_sim_wake();
#endif
