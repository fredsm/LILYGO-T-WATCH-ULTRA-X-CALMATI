#include "clock.hpp"
#include "navigation.hpp"
#include "screens.hpp"
// Original firmware accesses the colon span's style directly. LVGL 9 keeps
// that struct definition in this header; the simulator pins the same version.
#include <src/widgets/span/lv_span_private.h>
#include "mock/LilyGoLib.h"
#include "../defcon_img.h"
#include "../boot_skull_img.h"
#include "../nfc_icon.h"
#include "../matrix_bg.h"
#include "../weather.h"
#include "../alarm.h"
#include "../stopwatch_screen.h"
#include "../timer_screen.h"
#include "../wardriver_screen.h"
#include "../meshtastic_screen.h"
#include "../meshtastic.h"
#include "../tools_screen.h"
#include "../time_screen.h"
#include "../stealth.h"
#include <ctime>
#include <cstdio>

// Host-only weather fixture. Never starts HTTP, Wi-Fi or a worker task.
namespace { bool weather_valid=false; uint32_t weather_at=0; }
bool weather_fetch_async(){weather_valid=true;weather_at=lv_tick_get();return true;}
bool weather_is_fetching(){return false;}
WeatherSample weather_get(){return {weather_valid,22.0f,55,1013,lv_tick_get()-weather_at};}
void clock_screen_show();

#include "original_clock.inc"

void clock_screen_get_local_time(struct tm* out){instance.rtc.getDateTime(out);}

namespace sim {
void home_gesture(lv_event_t* e){
    if(stealth_active())return;
    auto* input=lv_event_get_indev(e);if(!input)return;
    auto dir=lv_indev_get_gesture_dir(input);
    if(dir==LV_DIR_TOP)tools_screen_show();
    else if(dir==LV_DIR_BOTTOM)show_quick_controls();
    else on_clock_gesture(e);
}
void power_button_click(){if(quick_controls_active())show_home();else original_power_click();}
void back_button_click(){
    static bool seen=false;static uint32_t last=0;
    const auto now=lv_tick_get();
    if(seen && now-last<250)return;
    seen=true;last=now;
    if(quick_controls_active())show_home();
    else if(time_screen_is_active())tools_screen_show();
    else original_back_click();
}
lv_obj_t* create_clock(){
    if(!clock_screen){
        create_original_clock();
        lv_obj_add_event_cb(clock_screen,home_gesture,LV_EVENT_GESTURE,nullptr);
        lv_obj_update_layout(clock_screen);
        realign_status_icons();
        update_clock();
        update_battery();
        layout_battery_indicators();
    }
    return clock_screen;
}
void refresh_clock(){
    if(!clock_screen)return;
    if(lv_tick_get()-last_update_ms>=1000){
        last_update_ms=lv_tick_get();
        update_clock();update_battery();layout_battery_indicators();
    }
}
}
