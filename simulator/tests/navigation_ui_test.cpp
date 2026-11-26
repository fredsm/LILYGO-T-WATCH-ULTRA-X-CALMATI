#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/sim/navigation.hpp"
#include "../../src/sim/mock/LilyGoLib.h"
#include "../../src/gps_screen.h"
#include "../../src/lora_screen.h"
#include "../../src/wifi_radio_screen.h"
#include "../../src/bluetooth_screen.h"
#include "../../src/settings_screen.h"
#include "../../src/ble_scan_manager.h"
#include "../../src/sim/mock/esp_bt.h"
#include "../../src/sim/mock/WiFi.h"
#include "../../src/pager.h"
#include "../../src/sim/mock/SD.h"
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
lv_obj_t* status_icon(lv_obj_t* obj,const char* name){
 const char* symbol=nullptr;
 if(!std::strcmp(name,"Wi-Fi"))symbol=LV_SYMBOL_WIFI;
 if(!std::strcmp(name,"Bluetooth"))symbol=LV_SYMBOL_BLUETOOTH;
 if(!std::strcmp(name,"GPS"))symbol=LV_SYMBOL_GPS;
 if(!std::strcmp(name,"SD"))symbol=LV_SYMBOL_SD_CARD;
 if(symbol && lv_obj_check_type(obj,&lv_label_class)
    && !std::strncmp(lv_label_get_text(obj),symbol,std::strlen(symbol)))return obj;
 if(!std::strcmp(name,"LoRa") && lv_obj_check_type(obj,&lv_arc_class)
    && lv_obj_get_width(obj)==22)return obj;
 if(!std::strcmp(name,"NFC") && lv_obj_check_type(obj,&lv_image_class)
    && lv_obj_get_width(obj)==26)return obj;
 for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)
  if(auto* found=status_icon(lv_obj_get_child(obj,i),name))return found;
 return nullptr;
}
bool highlighted(lv_obj_t* home,const char* name,bool on){
 auto* icon=status_icon(home,name);if(!icon)return false;
 auto color=!std::strcmp(name,"LoRa")?lv_obj_get_style_arc_color(icon,LV_PART_INDICATOR)
     : !std::strcmp(name,"NFC")?lv_obj_get_style_image_recolor(icon,LV_PART_MAIN)
     : lv_obj_get_style_text_color(icon,LV_PART_MAIN);
 return lv_color_to_u32(color)==lv_color_to_u32(lv_color_hex(on?0x00ff80:0x333333));
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
  swipe(205,100,0,240);if(!check(sim::quick_controls_active(),"clock down -> quick controls"))return 5;
  swipe(5,400,0,-300);if(!check(lv_screen_active()==home,"quick controls up -> clock"))return 6;
  swipe(205,330,0,-240);if(!check(tools_screen_is_active(),"clock up -> tools"))return 7;
  swipe(5,80,0,300);if(!check(lv_screen_active()==home,"tools edge down -> clock"))return 8;
 }
 swipe(205,330,0,-240);
 auto* grid=lv_obj_get_child(lv_screen_active(),1);
 swipe(205,400,0,-240);
 if(!check(tools_screen_is_active() && lv_obj_get_scroll_top(grid)>0,"tools inward swipe scrolls tiles"))return 12;
 sim::show_home();pump();
 swipe(205,330,0,-240);
 auto* time=label(lv_screen_active(),"Time");if(!check(time!=nullptr,"Time tile exists"))return 20;
 auto* tile=lv_obj_get_parent(time);if(!check(lv_obj_get_index(tile)==0,"Time is first tile"))return 21;
 lv_obj_scroll_to_view(tile,LV_ANIM_OFF);pump();lv_area_t t;lv_obj_get_coords(tile,&t);
 sample((t.x1+t.x2)/2,(t.y1+t.y2)/2,true);sample((t.x1+t.x2)/2,(t.y1+t.y2)/2,false);pump();
 if(!check(time_screen_is_active(),"tap Time tile"))return 22;
 swipe(5,80,0,300);if(!check(tools_screen_is_active(),"Time down returns to Tools"))return 23;
 time=label(lv_screen_active(),"Time");lv_obj_send_event(lv_obj_get_parent(time),LV_EVENT_CLICKED,nullptr);pump();
 auto* text=label(lv_screen_active(),"Stopwatch");if(!check(text!=nullptr,"original time tile exists"))return 9;
 lv_obj_scroll_to_view(text,LV_ANIM_OFF);pump();lv_area_t area;lv_obj_get_coords(text,&area);
 int x=(area.x1+area.x2)/2,y=(area.y1+area.y2)/2;
 sample(x,y,true);sample(x,y,false);pump();
 if(!check(stopwatch_screen_is_active(),"tap original stopwatch tile"))return 10;
 sim::show_home();pump();if(!check(lv_screen_active()==home,"home after nested screen"))return 11;
 sim::show_quick_controls();pump();
 struct Toggle {const char* name;bool (*on)();};
 const Toggle toggles[]={{"Wi-Fi",wifi_radio_screen_is_powered},{"Bluetooth",bluetooth_screen_is_powered},
     {"GPS",gps_screen_is_powered},{"LoRa",lora_screen_is_powered},{"NFC",[](){return instance.pmu.isEnableDLDO1();}}};
 for(const auto& toggle:toggles){
  auto* name=label(lv_screen_active(),toggle.name);if(!name)return 24;
  auto* button=lv_obj_get_parent(name);lv_area_t a;lv_obj_get_coords(button,&a);int x=(a.x1+a.x2)/2,y=(a.y1+a.y2)/2;
  for(int i=0;i<2;++i){bool before=toggle.on();sample(x,y,true);sample(x,y,false);pump();
   if(!check(toggle.on()!=before,toggle.name)||!sim::quick_controls_active())return 25;
   sim::show_home();for(int j=0;j<4;++j)pump();
   if(!check(highlighted(home,toggle.name,toggle.on()),"home icon follows toggle"))return 31;
   sim::show_quick_controls();pump();}

 }
 // Boot auto-connect and tools may start Wi-Fi without visiting its status page.
 WiFi.mode(WIFI_STA);pump();
 auto* wifi=label(lv_screen_active(),"Wi-Fi");
 if(!check(label(lv_obj_get_parent(wifi),"ON")!=nullptr,"Wi-Fi external enable reflected"))return 38;
 lv_obj_send_event(lv_obj_get_parent(wifi),LV_EVENT_CLICKED,nullptr);pump();
 if(!check(WiFi.current==WIFI_MODE_NULL,"Wi-Fi shortcut disables external enable"))return 39;
 ble_stack_acquire();
 auto* bt=label(lv_screen_active(),"Bluetooth");
 lv_obj_send_event(lv_obj_get_parent(bt),LV_EVENT_CLICKED,nullptr);pump();
 if(!check(esp_bt_controller_get_status()==ESP_BT_CONTROLLER_STATUS_ENABLED && label(lv_screen_active(),"Radio still used by another tool"),"Bluetooth shared owner retained"))return 36;
 ble_stack_release();pump();
 pager_start(433.92f,PAGER_POCSAG_1200);
 auto* lora=label(lv_screen_active(),"LoRa");
 lv_obj_send_event(lv_obj_get_parent(lora),LV_EVENT_CLICKED,nullptr);pump();
 if(!check(pager_is_running() && label(lv_screen_active(),"Radio still used by another tool"),"LoRa shared owner retained"))return 37;
 pager_stop();pump();
 auto* brightness=label(lv_screen_active(),"Brightness");if(!brightness)return 26;
 for(int expected:{85,170,255}){lv_obj_send_event(lv_obj_get_parent(brightness),LV_EVENT_CLICKED,nullptr);pump();if(sim::hardware.watch.brightness()!=expected || settings_get_brightness()!=expected)return 27;
  auto saved=sim_sd_files.find("/Settings/settings.txt");
  if(saved==sim_sd_files.end() || saved->second->find("brightness="+std::to_string(expected))==std::string::npos)return 35;}
 sim::back_button_click();pump();if(!check(lv_screen_active()==home,"Back closes shortcuts"))return 28;
 sim::show_quick_controls();pump();sample(205,425,true);sample(205,425,false);pump();
 if(!check(settings_screen_is_active(),"Settings shortcut opens original settings"))return 29;
 sim::back_button_click();pump();if(!check(lv_screen_active()==home,"Settings Back returns home"))return 30;
 for(bool ready:{false,true}){
  sim::hardware.sd_ready=ready;for(int j=0;j<4;++j)pump();
  if(!check(highlighted(home,"SD",ready),"home SD icon follows simulated card"))return 32;
 }
 gps_screen_power_on();sim::hardware.satellites=12;for(int j=0;j<4;++j)pump();
 if(!check(label(home,LV_SYMBOL_GPS " 12")!=nullptr,"home GPS satellite count"))return 33;
 gps_screen_power_off();pump();
 if(!check(label(home,LV_SYMBOL_GPS)!=nullptr && highlighted(home,"GPS",false),"GPS off clears count and highlight"))return 34;
 lv_indev_delete(input);lv_deinit();return 0;
}
