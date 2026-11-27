#pragma once
#include <lvgl.h>

// PIN authentication is independent of the power manager's temporary input lock.
void pin_lock_init(lv_obj_t* clock);
void pin_lock_restore();        // reload internal flash state on boot
void pin_lock_activate();
bool pin_lock_active();
bool pin_lock_unlock_screen_active();
bool pin_lock_handle_button(); // consumes physical button navigation while locked
void pin_lock_on_screenoff();  // discard partial PIN and return to locked clock
void pin_lock_enforce();       // reject incidental navigation while locked

// Shared padlock artwork for the clock status indicator and shortcut.
lv_obj_t* pin_lock_icon_create(lv_obj_t* parent, lv_color_t color, int size);
