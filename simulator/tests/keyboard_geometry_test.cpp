#include "../../src/keyboard_manager.h"
#include "../../src/watch_display_geometry.h"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/onboarding.h"
#include <src/widgets/buttonmatrix/lv_buttonmatrix_private.h>
#include <array>
#include <cstdio>
#include <cstring>

#define CHECK(x) do { if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;} } while(0)
namespace {
std::array<uint16_t,410*32> pixels{};
lv_indev_t* input=nullptr;
lv_point_t pointer{};
bool pressed=false;
void pump(int ms=100){for(int i=0;i<ms;i+=20){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
lv_obj_t* find(lv_obj_t* root,const lv_obj_class_t* type,const char* text=nullptr){
    if(lv_obj_has_flag(root,LV_OBJ_FLAG_HIDDEN))return nullptr;
    if(lv_obj_check_type(root,type)){
        if(!text)return root;
        if(type==&lv_label_class&&!strcmp(lv_label_get_text(root),text))return root;
        if(type==&lv_textarea_class&&!strcmp(lv_textarea_get_placeholder_text(root),text))return root;
    }
    for(unsigned i=0;i<lv_obj_get_child_count(root);++i)if(auto* child=find(lv_obj_get_child(root,i),type,text))return child;
    return nullptr;
}
bool click(const char* label){
    auto* l=find(lv_screen_active(),&lv_label_class,label);if(!l)return false;
    lv_obj_send_event(lv_obj_get_parent(l),LV_EVENT_CLICKED,nullptr);pump();return true;
}
bool open(const char* name){
    sim::show_home();size_t n=0;const auto* screens=sim::screens(n);
    for(size_t i=0;i<n;++i)if(!strcmp(screens[i].name,name)){screens[i].show();pump();return true;}
    return false;
}
lv_obj_t* field(const char* placeholder=nullptr){return find(lv_screen_active(),&lv_textarea_class,placeholder);}
lv_obj_t* keyboard(){return find(lv_screen_active(),&lv_keyboard_class);}
void focus(lv_obj_t* ta){if(ta){lv_obj_send_event(ta,LV_EVENT_FOCUSED,nullptr);pump();}}
bool inside(const lv_area_t& a){
    return watch_display_contains(a.x1,a.y1)&&watch_display_contains(a.x2,a.y1)&&
        watch_display_contains(a.x1,a.y2)&&watch_display_contains(a.x2,a.y2);
}
bool same(lv_area_t a,lv_area_t b){return a.x1==b.x1&&a.x2==b.x2&&a.y1==b.y1&&a.y2==b.y2;}
bool safe(const char* name,lv_keyboard_mode_t mode){
    auto* kb=keyboard();
    if(!kb||lv_keyboard_get_mode(kb)!=mode){std::fprintf(stderr,"%s: missing keyboard or wrong mode\n",name);return false;}
    auto* ta=lv_keyboard_get_textarea(kb);
    if(!ta||!lv_obj_is_visible(ta))return false;
    lv_obj_update_layout(lv_screen_active());
    lv_area_t k,a;lv_obj_get_coords(kb,&k);lv_obj_get_coords(ta,&a);
    if(!inside(k)||!inside(a)||k.x1<WATCH_KEYBOARD_SIDE_INSET||k.x2>=WATCH_DISPLAY_WIDTH-WATCH_KEYBOARD_SIDE_INSET||
       k.y2>=WATCH_DISPLAY_HEIGHT-WATCH_KEYBOARD_BOTTOM_INSET||a.y2>=k.y1){
        std::fprintf(stderr,"%s: unsafe keyboard (%d,%d)-(%d,%d), field (%d,%d)-(%d,%d)\n",
            name,int(k.x1),int(k.y1),int(k.x2),int(k.y2),int(a.x1),int(a.y1),int(a.x2),int(a.y2));return false;
    }
    auto* matrix=reinterpret_cast<lv_buttonmatrix_t*>(kb);
    for(unsigned i=0;i<matrix->btn_cnt;++i){
        auto b=matrix->button_areas[i];b.x1+=k.x1;b.x2+=k.x1;b.y1+=k.y1;b.y2+=k.y1;
        if(!inside(b)||lv_area_get_width(&b)<20||lv_area_get_height(&b)<35){
            std::fprintf(stderr,"%s: clipped or tiny key %s\n",name,lv_buttonmatrix_get_button_text(kb,i));return false;
        }
    }
    std::printf("%s safe (%u keys)\n",name,unsigned(matrix->btn_cnt));
    return true;
}
bool tap_key(const char* text){
    auto* kb=keyboard();if(!kb)return false;
    auto* matrix=reinterpret_cast<lv_buttonmatrix_t*>(kb);
    lv_area_t k;lv_obj_get_coords(kb,&k);
    for(unsigned i=0;i<matrix->btn_cnt;++i){
        if(strcmp(lv_buttonmatrix_get_button_text(kb,i),text))continue;
        auto b=matrix->button_areas[i];
        pointer={k.x1+(b.x1+b.x2)/2,k.y1+(b.y1+b.y2)/2};
        if(!watch_display_contains(pointer.x,pointer.y))return false;
        pressed=true;lv_tick_inc(30);sim::dispatch_pointer(input,true);
        pressed=false;lv_tick_inc(30);sim::dispatch_pointer(input,false);pump();return true;
    }
    return false;
}
}
int main(){
    lv_init();auto* d=lv_display_create(410,502);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,pixels.data(),nullptr,sizeof(pixels),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);
    lv_indev_set_read_cb(input,[](lv_indev_t*,lv_indev_data_t* data){data->point=pointer;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;});
    sim::initialize_screens(sim::create_clock());pump();
    CHECK(!watch_display_contains(0,0)&&!watch_display_contains(409,501)&&watch_display_contains(205,501));
    CHECK(open("send_message"));CHECK(click("+ Compose..."));CHECK(safe("Compose",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key("q")&&tap_key("p"));CHECK(!strcmp(lv_textarea_get_text(field()),"qp"));
    CHECK(tap_key(LV_SYMBOL_BACKSPACE));CHECK(!strcmp(lv_textarea_get_text(field()),"q"));
    CHECK(tap_key("ABC"));CHECK(safe("Uppercase",LV_KEYBOARD_MODE_TEXT_UPPER));
    CHECK(tap_key("A"));
    CHECK(tap_key("1#"));CHECK(safe("Symbols",LV_KEYBOARD_MODE_SPECIAL));
    CHECK(tap_key("/")&&tap_key("@"));CHECK(!strcmp(lv_textarea_get_text(field()),"qA/@"));
    CHECK(tap_key(LV_SYMBOL_KEYBOARD));CHECK(!keyboard());
    CHECK(open("wifi"));CHECK(click("SCAN"));pump(1200);CHECK(click("SIM"));
    CHECK(safe("Wi-Fi password",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(lv_textarea_get_password_mode(field()));
    CHECK(tap_key(LV_SYMBOL_KEYBOARD));CHECK(!keyboard());
    CHECK(open("configuration"));focus(field());CHECK(safe("Mesh configuration",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());
    CHECK(open("nfc_write"));focus(field());CHECK(safe("NFC text",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());
    CHECK(open("aprs"));focus(field());CHECK(safe("APRS",LV_KEYBOARD_MODE_TEXT_UPPER));
    CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());
    CHECK(open("portscan"));CHECK(click("Range"));focus(field());
    CHECK(safe("Port range",LV_KEYBOARD_MODE_NUMBER));CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());
    CHECK(open("pager"));focus(field("152.240"));CHECK(safe("Pager RX",LV_KEYBOARD_MODE_NUMBER));
    CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());CHECK(click("TX"));
    auto* tx_msg=field("Page text (leave empty for tone-only)");CHECK(tx_msg);
    lv_area_t before,after;lv_obj_get_coords(tx_msg,&before);
    focus(tx_msg);CHECK(safe("Pager TX text",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_OK));lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(tx_msg,&after);CHECK(same(before,after));
    focus(field("1234567"));CHECK(safe("Pager TX number",LV_KEYBOARD_MODE_NUMBER));CHECK(tap_key(LV_SYMBOL_OK));
    CHECK(open("phantom_flood"));focus(field());CHECK(safe("Phantom name",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_KEYBOARD));CHECK(!keyboard());
    CHECK(open("settings"));
    auto* url=find(lv_screen_active(),&lv_label_class,"Layout Source URL");CHECK(url);
    lv_obj_scroll_to_view_recursive(lv_obj_get_parent(url),LV_ANIM_OFF);pump();
    CHECK(click("Layout Source URL"));CHECK(safe("Source URL",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_KEYBOARD));CHECK(!keyboard());
    onboarding_screen_create();onboarding_screen_show();pump();
    auto* email=field("you@example.com");CHECK(email);lv_obj_get_coords(email,&before);
    focus(email);CHECK(safe("Onboarding email",LV_KEYBOARD_MODE_TEXT_LOWER));
    CHECK(tap_key(LV_SYMBOL_OK));CHECK(!keyboard());
    lv_obj_update_layout(lv_screen_active());lv_obj_get_coords(email,&after);CHECK(same(before,after));
    focus(email);CHECK(safe("Onboarding re-opened",LV_KEYBOARD_MODE_TEXT_LOWER));
    sim::show_home();pump();onboarding_screen_show();pump();
    CHECK(!keyboard());lv_obj_get_coords(email,&after);CHECK(same(before,after));
    // Dynamically removed fields/keyboards (future tools) must not leave a
    // stale textarea pointer or a moved field behind.
    auto* temporary=lv_obj_create(nullptr);lv_screen_load(temporary);
    auto* low=lv_textarea_create(temporary);lv_obj_set_size(low,380,44);lv_obj_align(low,LV_ALIGN_BOTTOM_MID,0,-50);
    auto* kb=watch_keyboard_create(temporary);lv_obj_update_layout(temporary);lv_obj_get_coords(low,&before);
    watch_keyboard_show(kb,low);pump();CHECK(safe("Dynamic editor",LV_KEYBOARD_MODE_TEXT_LOWER));
    lv_obj_delete(kb);lv_obj_update_layout(temporary);lv_obj_get_coords(low,&after);CHECK(same(before,after));
    kb=watch_keyboard_create(temporary);watch_keyboard_show(kb,low);pump();
    lv_obj_delete(low);CHECK(lv_obj_has_flag(kb,LV_OBJ_FLAG_HIDDEN)&&!lv_keyboard_get_textarea(kb));
    sim::show_home();lv_obj_delete(temporary);lv_deinit();
    return 0;
}
