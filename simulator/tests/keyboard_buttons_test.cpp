#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/keyboard.hpp"
#include "../../src/gps_screen.h"
#include "../../src/lora_screen.h"
#include "../../src/wifi_radio_screen.h"
#include "../../src/bluetooth_screen.h"
#include "../../src/nfc_screen.h"
#include "../../src/settings_screen.h"
#include "../../src/send_message_screen.h"
#include <array>
#include <cstring>
#include <cstdio>
namespace {
std::array<uint16_t,410*32> buffer{};
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
void pump(){for(int i=0;i<16;++i){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
lv_obj_t* find(lv_obj_t* obj,const lv_obj_class_t* type,const char* text=nullptr){
 if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN))return nullptr;
 if(lv_obj_check_type(obj,type) && (!text || !std::strcmp(lv_label_get_text(obj),text)))return obj;
 for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)if(auto* result=find(lv_obj_get_child(obj,i),type,text))return result;
 return nullptr;
}
bool compose(){auto* text=find(lv_screen_active(),&lv_label_class,"+ Compose...");if(!text)return false;lv_obj_send_event(lv_obj_get_parent(text),LV_EVENT_CLICKED,nullptr);pump();return sim::text_input_active();}
bool check(bool good,const char* name){std::printf("%s: %s\n",name,good?"PASS":"FAIL");std::fflush(stdout);return good;}
}
int main(){
 lv_init();auto* d=lv_display_create(410,502);
 lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(d,flush);
 auto* home=sim::create_clock();sim::initialize_screens(home);pump();
 sim::power_button_click();pump();if(!check(gps_screen_is_active(),"power clock -> GPS"))return 1;
 sim::power_button_click();pump();if(!check(lora_screen_is_active(),"power GPS -> LoRa"))return 2;
 sim::power_button_click();pump();if(!check(wifi_radio_screen_is_active(),"power LoRa -> WiFi"))return 3;
 sim::power_button_click();pump();if(!check(bluetooth_screen_is_active(),"power WiFi -> Bluetooth"))return 4;
 sim::power_button_click();pump();if(!check(nfc_screen_is_active(),"power Bluetooth -> NFC"))return 5;
 sim::back_button_click();if(!check(bluetooth_screen_is_active(),"back NFC -> Bluetooth"))return 6;
 sim::back_button_click();if(!check(bluetooth_screen_is_active(),"back debounce"))return 7;
 pump();sim::back_button_click();pump();if(!check(wifi_radio_screen_is_active(),"back Bluetooth -> WiFi"))return 8;
 sim::show_home();pump();sim::back_button_click();pump();if(!check(settings_screen_is_active(),"back clock -> settings"))return 9;
 sim::power_button_click();pump();if(!check(lv_screen_active()==home,"power settings -> clock"))return 10;
 send_message_screen_show();pump();if(!check(compose(),"open original compose keyboard"))return 11;
 auto* area=find(lv_screen_active(),&lv_textarea_class);if(!area)return 12;
 sim::text_input("pbM caf\xc3\xa9");
 if(!check(!std::strcmp(lv_textarea_get_text(area),"pbM caf\xc3\xa9") && send_message_screen_is_active(),"UTF-8 text entry"))return 13;
 sim::edit_key(LV_KEY_BACKSPACE);
 if(!check(!std::strcmp(lv_textarea_get_text(area),"pbM caf"),"UTF-8 backspace"))return 14;
 sim::edit_key(LV_KEY_HOME);sim::edit_key(LV_KEY_DEL);
 if(!check(!std::strcmp(lv_textarea_get_text(area),"bM caf"),"Home and Delete"))return 15;
 sim::edit_key(LV_KEY_ENTER);pump();
 if(!check(!sim::text_input_active() && send_message_screen_is_active(),"Enter submits original compose action"))return 16;
 if(!check(!sim::text_input("hidden"),"hidden editor rejects text"))return 17;
 if(!compose())return 18;sim::text_input("cancel me");sim::edit_key(LV_KEY_ESC);pump();
 if(!check(!sim::text_input_active() && send_message_screen_is_active(),"Escape cancels without exiting"))return 19;
 lv_deinit();return 0;
}
