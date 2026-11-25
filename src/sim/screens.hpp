#pragma once
#include <lvgl.h>
#include <cstddef>
namespace sim {
struct Screen {const char* name; void (*create)(); void (*show)(); bool (*active)(); void (*stop)();};
const Screen* screens(std::size_t& count);
void initialize_screens(lv_obj_t* home);
void tick();
void leave_screen();
void show_home();
}
