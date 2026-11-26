#pragma once
#include <lvgl.h>
namespace sim {
void initialize_navigation();
void show_quick_controls();
bool quick_controls_active();
void refresh_quick_controls();
void home_gesture(lv_event_t* e);
}
