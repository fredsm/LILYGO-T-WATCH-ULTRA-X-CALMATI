#include "navigation.hpp"
#include "screens.hpp"
#include "mock/LilyGoLib.h"
#include "../tools_screen.h"
#include "../time_screen.h"
#include "../gps_screen.h"
#include "../lora_screen.h"
#include "../wifi_radio_screen.h"
#include "../bluetooth_screen.h"
#include "../nfc_screen.h"
#include "../settings_screen.h"
#include <cstdint>
namespace {
lv_obj_t* quick=nullptr;
lv_obj_t* notice=nullptr;
lv_obj_t* brightness_value=nullptr;
lv_obj_t* time_tile=nullptr;
void tile_icons_passthrough(lv_obj_t* obj){
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i){
        auto* child=lv_obj_get_child(obj,i);lv_obj_clear_flag(child,LV_OBJ_FLAG_CLICKABLE);
        tile_icons_passthrough(child);
    }
}
bool nfc_on(){return instance.pmu.isEnableDLDO1();}
struct Control {
    const char* name;const char* icon;void (*show)();bool (*on)();
    lv_obj_t* original_switch=nullptr;lv_obj_t* tile=nullptr;lv_obj_t* value=nullptr;
};
Control controls[]={
    {"Wi-Fi",LV_SYMBOL_WIFI,wifi_radio_screen_show,wifi_radio_screen_is_powered},
    {"Bluetooth",LV_SYMBOL_BLUETOOTH,bluetooth_screen_show,bluetooth_screen_is_powered},
    {"GPS",LV_SYMBOL_GPS,gps_screen_show,gps_screen_is_powered},
    {"LoRa",LV_SYMBOL_CALL,lora_screen_show,lora_screen_is_powered},
    {"NFC",LV_SYMBOL_CHARGE,nfc_screen_show,nfc_on},
};
lv_obj_t* find_switch(lv_obj_t* obj){
    if(lv_obj_check_type(obj,&lv_switch_class))return obj;
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)if(auto* s=find_switch(lv_obj_get_child(obj,i)))return s;
    return nullptr;
}
void toggle(lv_event_t* e){
    auto* c=static_cast<Control*>(lv_event_get_user_data(e));
    if(!c->original_switch)return;
    bool desired=!c->on();
    if(desired)lv_obj_add_state(c->original_switch,LV_STATE_CHECKED);
    else lv_obj_remove_state(c->original_switch,LV_STATE_CHECKED);
    // Invoke the original radio screen's control callback, using host adapters.
    lv_obj_send_event(c->original_switch,LV_EVENT_VALUE_CHANGED,nullptr);
    lv_label_set_text(notice,!desired&&c->on()?"Radio still used by another tool":"Swipe up to close");
    sim::refresh_quick_controls();
}
lv_obj_t* make_control(int index,const char* name,const char* icon,lv_obj_t** value){
    auto* tile=lv_button_create(quick);
    lv_obj_set_pos(tile,35+(index%2)*180,76+(index/2)*110);lv_obj_set_size(tile,160,98);
    lv_obj_set_style_radius(tile,18,0);lv_obj_set_style_pad_all(tile,6,0);
    lv_obj_set_style_bg_color(tile,lv_color_hex(0x172331),0);
    lv_obj_set_style_border_width(tile,1,0);lv_obj_set_style_border_color(tile,lv_color_hex(0x344657),0);
    auto* glyph=lv_label_create(tile);lv_label_set_text(glyph,icon);
    lv_obj_set_style_text_font(glyph,&lv_font_montserrat_28,0);lv_obj_align(glyph,LV_ALIGN_TOP_MID,0,0);
    auto* label=lv_label_create(tile);lv_label_set_text(label,name);
    lv_obj_set_style_text_font(label,&lv_font_montserrat_16,0);lv_obj_align(label,LV_ALIGN_CENTER,0,9);
    *value=lv_label_create(tile);lv_obj_align(*value,LV_ALIGN_BOTTOM_MID,0,0);
    tile_icons_passthrough(tile);return tile;
}
void add_time_tile(lv_obj_t* grid){
    time_tile=lv_obj_create(grid);lv_obj_set_size(time_tile,180,180);
    lv_obj_set_style_bg_color(time_tile,lv_color_black(),0);lv_obj_set_style_bg_opa(time_tile,LV_OPA_COVER,0);
    lv_obj_set_style_radius(time_tile,14,0);lv_obj_set_style_pad_all(time_tile,0,0);
    lv_obj_set_style_border_width(time_tile,1,0);lv_obj_set_style_border_color(time_tile,lv_color_hex(0x004400),0);
    lv_obj_clear_flag(time_tile,LV_OBJ_FLAG_SCROLLABLE);
    auto* label=lv_label_create(time_tile);lv_label_set_text(label,"Time");
    lv_obj_set_style_text_font(label,&lv_font_montserrat_20,0);lv_obj_set_style_text_color(label,lv_color_hex(0x00cc00),0);
    lv_obj_align(label,LV_ALIGN_BOTTOM_MID,0,-12);
    auto* face=lv_obj_create(time_tile);lv_obj_set_size(face,88,88);lv_obj_align(face,LV_ALIGN_TOP_MID,0,24);
    lv_obj_set_style_radius(face,LV_RADIUS_CIRCLE,0);lv_obj_set_style_bg_color(face,lv_color_hex(0x101b25),0);
    lv_obj_set_style_border_color(face,lv_color_hex(0x65caff),0);lv_obj_set_style_border_width(face,3,0);
    lv_obj_set_style_pad_all(face,0,0);lv_obj_clear_flag(face,LV_OBJ_FLAG_SCROLLABLE);
    static const lv_point_precise_t hands[]={{41,16},{41,41},{62,53}};
    auto* hand=lv_line_create(face);lv_line_set_points(hand,hands,3);lv_obj_set_style_line_color(hand,lv_color_white(),0);
    lv_obj_set_style_line_width(hand,4,0);lv_obj_set_style_line_rounded(hand,true,0);
    tile_icons_passthrough(time_tile);
    lv_obj_add_event_cb(time_tile,[](lv_event_t*){time_screen_show();},LV_EVENT_CLICKED,nullptr);
    lv_obj_move_to_index(time_tile,0);
}
}
namespace sim {
void initialize_navigation(){
    if(quick)return;
    auto* previous=lv_screen_active();
    // Bind to the same switches used in the original hardware-status pages.
    for(auto& c:controls){c.show();c.original_switch=find_switch(lv_screen_active());}
    tools_screen_show();auto* tools=lv_screen_active();
    // Identify the original tile grid by its first tile, without assuming a child index.
    for(unsigned i=0;i<lv_obj_get_child_count(tools);++i){
        auto* child=lv_obj_get_child(tools,i);
        if(lv_obj_get_child_count(child)>10){add_time_tile(child);break;}
    }
    quick=lv_obj_create(nullptr);lv_obj_clear_flag(quick,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(quick,lv_color_hex(0x080e17),0);lv_obj_set_style_pad_all(quick,0,0);
    lv_obj_set_style_border_width(quick,0,0);lv_obj_set_style_text_color(quick,lv_color_white(),0);
    auto* title=lv_label_create(quick);lv_label_set_text(title,"QUICK CONTROLS");
    lv_obj_set_style_text_font(title,&lv_font_montserrat_20,0);lv_obj_align(title,LV_ALIGN_TOP_MID,0,32);
    for(int i=0;i<5;++i){auto& c=controls[i];c.tile=make_control(i,c.name,c.icon,&c.value);
        lv_obj_add_event_cb(c.tile,toggle,LV_EVENT_CLICKED,&c);
        if(!c.original_switch)lv_obj_add_state(c.tile,LV_STATE_DISABLED);
    }
    auto* brightness=make_control(5,"Brightness",LV_SYMBOL_EYE_OPEN,&brightness_value);
    lv_obj_add_event_cb(brightness,[](lv_event_t*){
        int value=hardware.watch.brightness();instance.setBrightness(value<100?170:value<200?255:85);
        refresh_quick_controls();
    },LV_EVENT_CLICKED,nullptr);
    auto* settings=lv_button_create(quick);lv_obj_set_size(settings,250,40);
    lv_obj_align(settings,LV_ALIGN_TOP_MID,0,405);lv_obj_set_style_radius(settings,14,0);
    lv_obj_set_style_bg_color(settings,lv_color_hex(0x26394c),0);
    auto* settings_label=lv_label_create(settings);
    lv_label_set_text(settings_label,LV_SYMBOL_SETTINGS "  Settings");lv_obj_center(settings_label);
    tile_icons_passthrough(settings);
    lv_obj_add_event_cb(settings,[](lv_event_t*){settings_screen_show();},LV_EVENT_CLICKED,nullptr);
    notice=lv_label_create(quick);lv_label_set_text(notice,"Swipe up to close");
    lv_obj_set_width(notice,340);lv_obj_set_style_text_align(notice,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_align(notice,LV_ALIGN_BOTTOM_MID,0,-20);
    lv_obj_add_event_cb(quick,[](lv_event_t* e){auto* input=lv_event_get_indev(e);
        if(input&&lv_indev_get_gesture_dir(input)==LV_DIR_TOP)show_home();
    },LV_EVENT_GESTURE,nullptr);
    refresh_quick_controls();lv_screen_load(previous);
}
void show_quick_controls(){leave_screen();refresh_quick_controls();lv_screen_load(quick);}
bool quick_controls_active(){return quick&&lv_screen_active()==quick;}
void refresh_quick_controls(){
    if(!quick)return;
    for(auto& c:controls){bool on=c.on();lv_label_set_text(c.value,on?"ON":"OFF");
        lv_obj_set_style_bg_color(c.tile,lv_color_hex(on?0x125348:0x172331),0);
        lv_obj_set_style_border_color(c.tile,lv_color_hex(on?0x3ce0b2:0x344657),0);
    }
    lv_label_set_text_fmt(brightness_value,"%d%%",hardware.watch.brightness()*100/255);
}
}
