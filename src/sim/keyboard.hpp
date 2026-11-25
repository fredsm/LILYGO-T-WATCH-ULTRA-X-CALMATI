#pragma once
#include <lvgl.h>
namespace sim {
bool text_input_active();
bool text_input(const char* utf8);
bool edit_key(uint32_t key);
}
