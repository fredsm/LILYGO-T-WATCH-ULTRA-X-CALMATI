#include "layout_config.h"
#include "storage_manager.h"
#include "power_manager.h"
#include "pin_lock_screen.h"
#include "stealth.h"
#include "time_screen.h"
#include "meshtastic.h"
#include "meshtastic_screen.h"
#include "tools_screen.h"
#include "quick_controls_screen.h"
#include "settings_screen.h"
#include "gps_screen.h"
#include "lora_screen.h"
#include "wifi_radio_screen.h"
#include "bluetooth_screen.h"
#include "nfc_screen.h"
#include "calendar_screen.h"
#include "stopwatch_screen.h"
#include "timer_screen.h"
#include "alarm_screen.h"
#include "diagnostics_screen.h"
#include "task_manager.h"
#include <SD.h>
#include <stdlib.h>

namespace {
LayoutConfig current=layout_defaults();
LayoutStatus status{LayoutSource::Embedded,false,false};
enum class ReadResult { Missing, Invalid, Loaded };
ReadResult read_layout(const char* path,LayoutConfig& base){
    if(!SD.exists(path))return ReadResult::Missing;
    auto f=SD.open(path,FILE_READ);
    if(!f)return ReadResult::Invalid;
    const size_t n=f.size();
    if(n>LAYOUT_MAX_BYTES){f.close();return ReadResult::Invalid;}
    auto* buffer=static_cast<char*>(storage_allocate(n+1));
    if(!buffer){f.close();return ReadResult::Invalid;}
    const auto got=f.read(reinterpret_cast<uint8_t*>(buffer),n);
    f.close();
    const bool ok=got==n&&layout_parse_overlay(buffer,n,base,base);
    free(buffer);
    return ok?ReadResult::Loaded:ReadResult::Invalid;
}
}
const LayoutConfig& layout_config(){return current;}
LayoutStatus layout_status(){return status;}
void layout_load(){
    current=layout_defaults();status={LayoutSource::Embedded,false,false};
    StorageAccess access;
    if(!access)return;
    const auto defaults=read_layout(LAYOUT_DEFAULT_PATH,current);
    if(defaults==ReadResult::Loaded)status.source=LayoutSource::DefaultFile;
    status.invalid_default=defaults==ReadResult::Invalid;
    const auto custom=read_layout(LAYOUT_PATH,current);
    if(custom==ReadResult::Loaded)status.source=LayoutSource::CustomFile;
    status.invalid_custom=custom==ReadResult::Invalid;
}
void layout_open_home(HomeInput input){
    if(power_screen_off()||power_shutting_down()||pin_lock_active()||stealth_active())return;
    const unsigned i=static_cast<unsigned>(input);
    if(i>=static_cast<unsigned>(HomeInput::Count))return;
    switch(current.home[i]){
        case HomeTarget::None:break;
        case HomeTarget::Time:time_screen_show_from_clock();break;
        case HomeTarget::Meshtastic:meshtastic_mark_read();meshtastic_screen_show();break;
        case HomeTarget::Tools:tools_screen_show();break;
        case HomeTarget::QuickControls:quick_controls_screen_show();break;
        case HomeTarget::Settings:settings_screen_show();break;
        case HomeTarget::Gps:gps_screen_show();break;
        case HomeTarget::LoRa:lora_screen_show();break;
        case HomeTarget::WifiRadio:wifi_radio_screen_show();break;
        case HomeTarget::Bluetooth:bluetooth_screen_show();break;
        case HomeTarget::Nfc:nfc_screen_show();break;
        case HomeTarget::Calendar:calendar_screen_show();break;
        case HomeTarget::Stopwatch:stopwatch_screen_show();break;
        case HomeTarget::Timer:timer_screen_show();break;
        case HomeTarget::Alarm:alarm_screen_show();break;
        case HomeTarget::Diagnostics:diagnostics_screen_show();break;
        case HomeTarget::TaskManager:task_manager_show();break;
    }
}
