#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/settings_screen.h"
#include <array>
#include <cstdio>
#include <cstring>
extern void clock_screen_show();
extern void clock_screen_set_analog_face(bool);
extern void clock_screen_set_12h(bool);
extern bool clock_screen_uses_12h();
namespace {
std::array<uint16_t,410*32> buffer{};
unsigned frames=0;
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){++frames;lv_display_flush_ready(d);}
bool label_contains(lv_obj_t* obj,const char* text){
 if(lv_obj_check_type(obj,&lv_label_class) && std::strstr(lv_label_get_text(obj),text))return true;
 for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)if(label_contains(lv_obj_get_child(obj,i),text))return true;
 return false;
}
void pump(){for(int i=0;i<60;++i){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
}
int main(){
 lv_init();auto* d=lv_display_create(410,502);
 lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(d,flush);
 auto* home=sim::create_clock();sim::initialize_screens(home);pump();
 if(lv_screen_active()!=home || !frames || !label_contains(home,"75%") || !label_contains(home,"DEF CON 35"))return 1;
 if(lv_obj_get_width(home)!=410 || lv_obj_get_height(home)!=502)return 2;
 clock_screen_set_12h(false);pump();if(clock_screen_uses_12h())return 3;
 clock_screen_set_analog_face(true);pump();if(!clock_screen_uses_12h())return 4;
 clock_screen_set_analog_face(false);clock_screen_set_12h(true);pump();
 settings_screen_show();pump();if(lv_screen_active()==home)return 5;
 clock_screen_show();pump();if(lv_screen_active()!=home)return 6;
 if(sim::create_clock()!=home)return 7;
 lv_deinit();std::puts("Original clock rendered, switched faces and returned home");return 0;
}
