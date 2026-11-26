#pragma once
#include <lvgl.h>

// Time utilities are reached from a right-to-left home swipe or the Tools tile.
void time_screen_create();
void time_screen_show();
bool time_screen_is_active();

void time_screen_show_from_clock();
void time_screen_show_from_tools();
void time_screen_back();
