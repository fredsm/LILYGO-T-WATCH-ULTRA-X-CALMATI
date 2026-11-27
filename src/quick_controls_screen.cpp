#include "quick_controls_screen.h"
#include "pin_lock_screen.h"
#include <LilyGoLib.h>
#include <esp_bt.h>
#include <esp_wifi.h>
#include "pager.h"
#include "tpms.h"
#include "aprs.h"
#include "lora_analyze_screen.h"
void clock_screen_show();
void main_loop_request_lvgl_priority(int);
#include "tools_screen.h"
#include "time_screen.h"
#include "gps_screen.h"
#include "lora_screen.h"
#include "wifi_radio_screen.h"
#include "bluetooth_screen.h"
#include "nfc_screen.h"
#include "settings_screen.h"
#include <cstdint>
namespace {
lv_obj_t* quick=nullptr;
lv_obj_t* notice=nullptr;
lv_obj_t* brightness_value=nullptr;
void tile_icons_passthrough(lv_obj_t* obj){
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i){
        auto* child=lv_obj_get_child(obj,i);lv_obj_clear_flag(child,LV_OBJ_FLAG_CLICKABLE);
        tile_icons_passthrough(child);
    }
}
bool wifi_on(){wifi_mode_t mode=WIFI_MODE_NULL;esp_wifi_get_mode(&mode);return mode!=WIFI_MODE_NULL;}
bool bluetooth_on(){return esp_bt_controller_get_status()==ESP_BT_CONTROLLER_STATUS_ENABLED;}
bool lora_on(){return lora_screen_is_powered() || pager_is_running() || tpms_is_running() || aprs_is_running() || lora_analyze_is_running();}
void gps_power(bool on){if(on)gps_screen_power_on();else gps_screen_power_off();}
bool nfc_on(){return instance.pmu.isEnableDLDO1();}
struct Control {
    const char* name;const char* icon;void (*set)(bool);bool (*on)();
    lv_obj_t* tile=nullptr;lv_obj_t* value=nullptr;
    Control(const char* n,const char* i,void (*s)(bool),bool (*o)()):name(n),icon(i),set(s),on(o){}
};
Control controls[]={
    {"Wi-Fi",LV_SYMBOL_WIFI,wifi_radio_screen_set_powered,wifi_on},
    {"Bluetooth",LV_SYMBOL_BLUETOOTH,bluetooth_screen_set_powered,bluetooth_on},
    {"GPS",LV_SYMBOL_GPS,gps_power,gps_screen_is_powered},
    {"LoRa",LV_SYMBOL_CALL,lora_screen_set_powered,lora_on},
    {"NFC",LV_SYMBOL_CHARGE,nfc_screen_set_powered,nfc_on},
};
void toggle(lv_event_t* e){
    auto* c=static_cast<Control*>(lv_event_get_user_data(e));
    bool desired=!c->on();
    c->set(desired);
    lv_label_set_text(notice,desired!=c->on()?(desired?"Could not enable radio":"Radio still used by another tool"):"Swipe up to close");
    quick_controls_screen_refresh();
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

}
void quick_controls_screen_create(){
    if(quick)return;
    quick=lv_obj_create(nullptr);lv_obj_clear_flag(quick,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(quick,lv_color_hex(0x080e17),0);lv_obj_set_style_pad_all(quick,0,0);
    lv_obj_set_style_border_width(quick,0,0);lv_obj_set_style_text_color(quick,lv_color_white(),0);
    auto* title=lv_label_create(quick);lv_label_set_text(title,"QUICK CONTROLS");
    lv_obj_set_style_text_font(title,&lv_font_montserrat_20,0);lv_obj_align(title,LV_ALIGN_TOP_MID,0,32);
    for(int i=0;i<5;++i){auto& c=controls[i];c.tile=make_control(i,c.name,c.icon,&c.value);
        lv_obj_add_event_cb(c.tile,toggle,LV_EVENT_CLICKED,&c);
    }
    auto* brightness=make_control(5,"Brightness",LV_SYMBOL_EYE_OPEN,&brightness_value);
    lv_obj_add_event_cb(brightness,[](lv_event_t*){
        int value=settings_get_brightness();
        settings_set_brightness(value<DEVICE_MAX_BRIGHTNESS_LEVEL*2/5 ? DEVICE_MAX_BRIGHTNESS_LEVEL*2/3 :
            value<DEVICE_MAX_BRIGHTNESS_LEVEL*4/5 ? DEVICE_MAX_BRIGHTNESS_LEVEL : DEVICE_MAX_BRIGHTNESS_LEVEL/3);
        quick_controls_screen_refresh();
    },LV_EVENT_CLICKED,nullptr);
    auto* settings=lv_button_create(quick);lv_obj_set_size(settings,160,40);
    lv_obj_set_pos(settings,35,405);lv_obj_set_style_radius(settings,14,0);
    lv_obj_set_style_bg_color(settings,lv_color_hex(0x26394c),0);
    auto* settings_label=lv_label_create(settings);
    lv_label_set_text(settings_label,LV_SYMBOL_SETTINGS "  Settings");lv_obj_center(settings_label);
    tile_icons_passthrough(settings);
    lv_obj_add_event_cb(settings,[](lv_event_t*){settings_screen_show();},LV_EVENT_CLICKED,nullptr);
    auto* lock=lv_button_create(quick);lv_obj_set_size(lock,160,40);
    lv_obj_set_pos(lock,215,405);lv_obj_set_style_radius(lock,14,0);
    lv_obj_set_style_bg_color(lock,lv_color_hex(0x26394c),0);
    auto* lock_icon=pin_lock_icon_create(lock,lv_color_white(),20);
    lv_obj_align(lock_icon,LV_ALIGN_LEFT_MID,24,0);
    auto* lock_label=lv_label_create(lock);
    lv_label_set_text(lock_label,"Lock");lv_obj_align(lock_label,LV_ALIGN_CENTER,10,0);
    tile_icons_passthrough(lock);
    lv_obj_add_event_cb(lock,[](lv_event_t*){pin_lock_activate();},LV_EVENT_CLICKED,nullptr);
    notice=lv_label_create(quick);lv_label_set_text(notice,"Swipe up to close");
    lv_obj_set_width(notice,340);lv_obj_set_style_text_align(notice,LV_TEXT_ALIGN_CENTER,0);
    lv_obj_align(notice,LV_ALIGN_BOTTOM_MID,0,-20);
    lv_obj_add_event_cb(quick,[](lv_event_t* e){auto* input=lv_event_get_indev(e);
        if(input&&lv_indev_get_gesture_dir(input)==LV_DIR_TOP)clock_screen_show();
    },LV_EVENT_GESTURE,nullptr);
    quick_controls_screen_refresh();
    lv_timer_create([](lv_timer_t*){if(quick_controls_screen_is_active())quick_controls_screen_refresh();},250,nullptr);
}
void quick_controls_screen_show(){if(!quick)quick_controls_screen_create();main_loop_request_lvgl_priority(8);quick_controls_screen_refresh();lv_screen_load(quick);}
bool quick_controls_screen_is_active(){return quick&&lv_screen_active()==quick;}
void quick_controls_screen_refresh(){
    if(!quick)return;
    for(auto& c:controls){bool on=c.on();lv_label_set_text(c.value,on?"ON":"OFF");
        lv_obj_set_style_bg_color(c.tile,lv_color_hex(on?0x125348:0x172331),0);
        lv_obj_set_style_border_color(c.tile,lv_color_hex(on?0x3ce0b2:0x344657),0);
    }
    lv_label_set_text_fmt(brightness_value,"%d%%",settings_get_brightness()*100/DEVICE_MAX_BRIGHTNESS_LEVEL);
}
