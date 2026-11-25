#include "clock.hpp"
#include "mock/LilyGoLib.h"
#include "../defcon_img.h"
#include "../nfc_icon.h"
#include "../matrix_bg.h"
#include "../weather.h"
#include "../alarm.h"
#include "../stopwatch_screen.h"
#include "../timer_screen.h"
#include <ctime>
#include <cstdio>

// Host-only weather fixture. Never starts HTTP, Wi-Fi or a worker task.
namespace { bool weather_valid=false; uint32_t weather_at=0; }
bool weather_fetch_async(){weather_valid=true;weather_at=lv_tick_get();return true;}
bool weather_is_fetching(){return false;}
WeatherSample weather_get(){return {weather_valid,22.0f,55,1013,lv_tick_get()-weather_at};}

#include "original_clock.inc"

void clock_screen_get_local_time(struct tm* out){instance.rtc.getDateTime(out);}

namespace sim {
lv_obj_t* create_clock(){
    if(!clock_screen){
        create_original_clock();
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
