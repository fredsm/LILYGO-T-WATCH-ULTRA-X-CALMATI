#include "../../src/sim/screens.hpp"
#include "../../src/portscan.h"
#include "../../src/ble_pair_audit.h"
#include "../../src/nfc_screen.h"
#include "../../src/nfc_write_screen.h"
#include "../../src/nfc_credit_screen.h"
#include "../../src/gatt_explorer_screen.h"
#include "../../src/voice_screen.h"
#include "../../src/badusb_screen.h"
#include "../../src/badusb.h"
#include "SD.h"
#include <array>
#include <cstdio>
#include <cstring>
namespace {
std::array<uint16_t,410*32> buffer{};
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
void pump(int ms=1000){for(int i=0;i<ms;i+=20){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
lv_obj_t* label(lv_obj_t* root,const char* text){
 if(lv_obj_check_type(root,&lv_label_class)&&std::strstr(lv_label_get_text(root),text))return root;
 for(unsigned i=0;i<lv_obj_get_child_count(root);++i)if(auto* found=label(lv_obj_get_child(root,i),text))return found;return nullptr;
}
bool click(const char* text){auto* l=label(lv_screen_active(),text);if(!l){std::fprintf(stderr,"Missing label: %s\n",text);return false;}lv_obj_send_event(lv_obj_get_parent(l),LV_EVENT_CLICKED,nullptr);return true;}
bool open(const char* name){std::size_t n;auto* entries=sim::screens(n);sim::show_home();for(size_t i=0;i<n;++i)if(!std::strcmp(name,entries[i].name)){entries[i].show();pump();return true;}return false;}
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"Check failed at %d: %s\n",__LINE__,#x);return 1;}}while(0)
}
int main(){
 lv_init();auto* d=lv_display_create(410,502);lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);lv_display_set_flush_cb(d,flush);
 sim::initialize_screens(lv_screen_active());
 CHECK(open("portscan"));CHECK(click("START"));pump();CHECK(portscan_result_count()==2);CHECK(!portscan_is_running());CHECK(label(lv_screen_active(),"80"));
 CHECK(open("ble_pair"));CHECK(bpa_count()==2);CHECK(click("CLEAR"));CHECK(bpa_count()==0);
 CHECK(open("badusb"));CHECK(click("RUN"));pump(2500);CHECK(!badusb_is_running());CHECK(badusb_progress()==100);
 CHECK(open("nfc"));lv_obj_t* sw=nullptr;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());++i){auto* o=lv_obj_get_child(lv_screen_active(),i);if(lv_obj_check_type(o,&lv_switch_class))sw=o;}
 CHECK(sw);lv_obj_add_state(sw,LV_STATE_CHECKED);lv_obj_send_event(sw,LV_EVENT_VALUE_CHANGED,nullptr);
 CHECK(click("Read"));pump();CHECK(label(lv_screen_active(),"SIM NFC tag"));
 nfc_write_screen_show();lv_obj_t* value=nullptr;for(unsigned i=0;i<lv_obj_get_child_count(lv_screen_active());++i){auto* o=lv_obj_get_child(lv_screen_active(),i);if(lv_obj_check_type(o,&lv_textarea_class))value=o;}
 CHECK(value);lv_textarea_set_text(value,"SIM round trip");CHECK(click("Write"));pump();CHECK(label(lv_screen_active(),"Written successfully"));
 nfc_screen_show();CHECK(click("Read"));pump();CHECK(label(lv_screen_active(),"SIM round trip"));
 CHECK(open("nfc_credit"));CHECK(click("PRIMA"));pump();CHECK(click("DOPO"));pump();CHECK(SD.exists("/frantic/verify_log.txt"));
 CHECK(open("gatt_explorer"));CHECK(click("senza nome"));pump();CHECK(label(lv_screen_active(),"SVC 0x180F"));
 CHECK(open("map"));lv_image_header_t header{};CHECK(lv_image_decoder_get_info("A:/map/1/0/0.png",&header)==LV_RESULT_OK);CHECK(header.w==256&&header.h==256);
 CHECK(open("voice"));CHECK(click("PARLA"));pump(11000);CHECK(label(lv_screen_active(),"Simulated assistant response."));
 sim::show_home();lv_deinit();std::puts("Original UI interaction flows passed");return 0;
}
