#include "../../src/sim/startup.hpp"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/mock/LilyGoLib.h"
#include "../../src/onboarding.h"
#include "../../src/sim/mock/SD.h"
#include <array>
#include <cstring>
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
std::array<uint16_t,410*32> buffer{};
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
void pump(int ms){for(int i=0;i<ms;i+=20){lv_tick_inc(20);sim::tick();sim::tick_boot();lv_timer_handler();}}
lv_obj_t* find(lv_obj_t* obj,const lv_obj_class_t* cls,const char* text=nullptr){
 if(lv_obj_check_type(obj,cls)&&(!text||std::strstr(lv_label_get_text(obj),text)))return obj;
 for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)if(auto* r=find(lv_obj_get_child(obj,i),cls,text))return r;
 return nullptr;
}
}
int main(){
 lv_init();auto* d=lv_display_create(410,502);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
 auto* home=sim::create_clock();sim::initialize_screens(home);
 CHECK(sim::hardware.watch.brightness()==255);
 sim::start_boot(false);auto* splash=lv_screen_active();CHECK(splash!=home&&sim::startup_active());
 CHECK(find(splash,&lv_image_class));CHECK(find(splash,&lv_label_class,"13:37"));
 pump(200);CHECK(lv_obj_get_style_opa(splash,LV_PART_MAIN)>0&&lv_obj_get_style_opa(splash,LV_PART_MAIN)<255);
 pump(1200);CHECK(lv_screen_active()==splash);pump(120);CHECK(lv_screen_active()==home&&!sim::startup_active());
 sim::hardware.watch.set_battery(14);sim::hardware.watch.set_charging(true);pump(1200);
 CHECK(instance.pmu.getBatteryPercent()==14&&instance.pmu.isCharging());CHECK(find(home,&lv_label_class,"14%"));
 instance.setBrightness(80);CHECK(sim::hardware.watch.brightness()==80);
 sim::hardware.gps_fix=false;CHECK(!instance.gps.location.isValid());
 sim::hardware.gps_fix=true;sim::hardware.satellites=3;CHECK(instance.gps.satellites.value()==3);
 auto file=SD.open("/presence-test",FILE_WRITE);CHECK(file&&file.print("test")==4);
 sim::hardware.sd_ready=false;CHECK(!file&&file.print("blocked")==0&&!SD.open("/presence-test"));
 CHECK(!instance.isCardReady());sim::hardware.reset_inputs();CHECK(instance.isCardReady());CHECK(SD.open("/presence-test").size()==4);
 CHECK(sim::hardware.watch.brightness()==255&&instance.pmu.getBatteryPercent()==75&&!instance.pmu.isCharging());
 sim::start_boot(true);pump(1520);auto* onboarding=lv_screen_active();CHECK(onboarding!=home&&sim::startup_active());
 auto* accept=find(onboarding,&lv_label_class,"ACCETTO / ACCEPT");auto* checkbox=find(onboarding,&lv_checkbox_class);CHECK(accept&&checkbox);
 lv_obj_send_event(lv_obj_get_parent(accept),LV_EVENT_CLICKED,nullptr);CHECK(lv_screen_active()==onboarding&&onboarding_needed());
 lv_obj_add_state(checkbox,LV_STATE_CHECKED);lv_obj_send_event(lv_obj_get_parent(accept),LV_EVENT_CLICKED,nullptr);
 CHECK(lv_screen_active()==home&&!onboarding_needed()&&!sim::startup_active());
 sim::start_boot(true);pump(1520);CHECK(lv_screen_active()==home);
 lv_deinit();std::puts("Original splash, onboarding, session consent and shared hardware: PASS");return 0;
}
