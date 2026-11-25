#pragma once
#include <lvgl.h>
namespace sim {
lv_obj_t* create_clock();
void refresh_clock();
void power_button_click();
void back_button_click();
}
