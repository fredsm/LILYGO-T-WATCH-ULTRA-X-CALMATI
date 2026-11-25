#include "../../src/calendar_screen.h"
#include <array>
#include <ctime>
#include <string_view>
static std::array<uint16_t,502*32> buf{};
static lv_obj_t* parent;
void time_screen_show(){lv_screen_load(parent);}
void clock_screen_get_local_time(struct tm* out){*out={};out->tm_year=124;out->tm_mon=1;out->tm_mday=29;}
static void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
int main(){
 lv_init();
 auto* d=lv_display_create(502,410);
 lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(d,flush);
 parent=lv_screen_active();
 calendar_screen_create();calendar_screen_show();
 if(!calendar_screen_is_active())return 1;
 auto* s=lv_screen_active();
 auto* title=lv_obj_get_child(s,3);
 if(std::string_view(lv_label_get_text(title))!="February 2024")return 2;
 lv_obj_send_event(lv_obj_get_child(s,2),LV_EVENT_CLICKED,nullptr);
 if(std::string_view(lv_label_get_text(title))!="March 2024")return 3;
 lv_obj_send_event(lv_obj_get_child(s,0),LV_EVENT_CLICKED,nullptr);
 if(std::string_view(lv_label_get_text(title))!="February 2024")return 4;
 lv_tick_inc(50);lv_timer_handler();
 time_screen_show();
 if(calendar_screen_is_active())return 5;
 lv_deinit();return 0;
}
