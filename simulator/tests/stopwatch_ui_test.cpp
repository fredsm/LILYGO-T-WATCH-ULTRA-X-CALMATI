#include <lvgl.h>
#include "../../src/stopwatch_screen.h"
#include <array>
#include <cstdint>
#include <cstdio>
#include <string_view>
static std::array<uint16_t,502*32> buf{};
static lv_obj_t* parent;
void time_screen_show() { lv_screen_load(parent); }
static void flush(lv_display_t* d,const lv_area_t*,uint8_t*) { lv_display_flush_ready(d); }
int main() {
    lv_init();
    auto* d=lv_display_create(502,410);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,flush);
    parent=lv_screen_active();
    stopwatch_screen_create();
    stopwatch_screen_show();
    if(!stopwatch_screen_is_active() || stopwatch_is_running()) return 1;
    auto* screen=lv_screen_active();
    // Original screen children: title, elapsed, lap label, lap, start, reset, list.
    auto* start=lv_obj_get_child(screen,4);
    if(!start) return 2;
    lv_obj_send_event(start,LV_EVENT_CLICKED,nullptr);
    if(!stopwatch_is_running()) return 3;
    lv_tick_inc(1250); lv_timer_handler();
    auto* elapsed=lv_obj_get_child(screen,1);
    if(std::string_view(lv_label_get_text(elapsed))!="0:01.25") return 4;
    lv_obj_send_event(start,LV_EVENT_CLICKED,nullptr);
    if(stopwatch_is_running()) return 5;
    time_screen_show();
    if(stopwatch_screen_is_active()) return 6;
    lv_deinit();
    return 0;
}
