#pragma once
#include <lvgl.h>

// One shared implementation, with screen-owned LVGL widgets so existing
// screen-specific callbacks and modal lifetimes remain independent.
enum class KeyboardProfile { Standard, Compact, Compose };
lv_obj_t* watch_keyboard_create(lv_obj_t* parent,KeyboardProfile profile=KeyboardProfile::Standard);
void watch_keyboard_show(lv_obj_t* keyboard,lv_obj_t* textarea);
void watch_keyboard_hide(lv_obj_t* keyboard);
