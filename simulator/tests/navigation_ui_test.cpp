#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/wardriver_screen.h"
#include "../../src/meshtastic_screen.h"
#include "../../src/tools_screen.h"
#include "../../src/time_screen.h"
#include "../../src/stopwatch_screen.h"
#include <array>
#include <cstdio>
#include <cstring>
namespace {
std::array<uint16_t,410*32> buffer{};
lv_indev_t* input=nullptr;lv_point_t point{};bool pressed=false;
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
void read(lv_indev_t*,lv_indev_data_t* d){d->point=point;d->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;}
void pump(){for(int i=0;i<15;++i){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
void sample(int x,int y,bool down){point={x,y};pressed=down;lv_tick_inc(20);sim::dispatch_pointer(input,pressed);lv_timer_handler();}
void swipe(int x,int y,int dx,int dy){
 sample(x,y,true);
 for(int i=1;i<=8;++i)sample(x+dx*i/8,y+dy*i/8,true);
 sample(x+dx,y+dy,false);pump();
}
lv_obj_t* label(lv_obj_t* parent,const char* text){
 if(lv_obj_check_type(parent,&lv_label_class)&&!std::strcmp(lv_label_get_text(parent),text))return parent;
 for(unsigned i=0;i<lv_obj_get_child_count(parent);++i)if(auto* result=label(lv_obj_get_child(parent,i),text))return result;
 return nullptr;
}
bool check(bool condition,const char* name){
 std::printf("%s: %s\n",name,condition?"PASS":"FAIL");
 if(!condition){std::size_t count=0;const auto* entries=sim::screens(count);for(std::size_t i=0;i<count;++i)if(entries[i].active())std::printf("Active: %s\n",entries[i].name);}
 std::fflush(stdout);return condition;
}
}
int main(){
 lv_init();auto* d=lv_display_create(410,502);
 lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(d,flush);
 input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
 lv_indev_set_read_cb(input,read);lv_indev_set_scroll_limit(input,35);
 lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);
 auto* home=sim::create_clock();sim::initialize_screens(home);pump();
 for(int pass=0;pass<2;++pass){
  swipe(330,250,-240,0);if(!check(wardriver_screen_is_active(),"clock left -> wardriver"))return 1;
  swipe(25,12,300,0);if(!check(lv_screen_active()==home,"wardriver right -> clock"))return 2;
  swipe(80,250,240,0);if(!check(meshtastic_screen_is_active(),"clock right -> mesh"))return 3;
  swipe(350,12,-300,0);if(!check(lv_screen_active()==home,"mesh left -> clock"))return 4;
  swipe(205,330,0,-240);if(!check(time_screen_is_active(),"clock up -> time"))return 5;
  swipe(5,80,0,300);if(!check(lv_screen_active()==home,"time down -> clock"))return 6;
  swipe(205,100,0,240);if(!check(tools_screen_is_active(),"clock down -> tools"))return 7;
  swipe(5,80,0,300);if(!check(lv_screen_active()==home,"tools edge down -> clock"))return 8;
 }
 swipe(205,330,0,-240);
 auto* text=label(lv_screen_active(),"Stopwatch");if(!check(text!=nullptr,"original time tile exists"))return 9;
 lv_obj_scroll_to_view(text,LV_ANIM_OFF);pump();lv_area_t area;lv_obj_get_coords(text,&area);
 int x=(area.x1+area.x2)/2,y=(area.y1+area.y2)/2;
 sample(x,y,true);sample(x,y,false);pump();
 if(!check(stopwatch_screen_is_active(),"tap original stopwatch tile"))return 10;
 sim::show_home();pump();if(!check(lv_screen_active()==home,"home after nested screen"))return 11;
 lv_indev_delete(input);lv_deinit();return 0;
}
