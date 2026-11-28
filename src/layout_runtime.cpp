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
#include <FFat.h>
#include <SD.h>
#include <string.h>
#include <stdlib.h>

namespace {
constexpr const char* backup_path="/Settings/layout.bak";
constexpr const char* temp_path="/Settings/layout.tmp";
LayoutConfig current=layout_defaults();
LayoutStatus status{LayoutSource::Embedded,false,false};
enum class ReadResult { Missing, Invalid, Loaded };
struct Text {
    char* data=nullptr;
    size_t size=0;
    Text()=default;
    ~Text(){free(data);}
    Text(const Text&)=delete;
    Text& operator=(const Text&)=delete;
};
template<class FS> ReadResult read_text(FS& fs,const char* path,Text& text){
    if(!fs.exists(path))return ReadResult::Missing;
    auto f=fs.open(path,FILE_READ);
    if(!f)return ReadResult::Invalid;
    const size_t n=f.size();
    if(n>LAYOUT_MAX_BYTES){f.close();return ReadResult::Invalid;}
    text.data=static_cast<char*>(storage_allocate(n+1));
    if(!text.data){f.close();return ReadResult::Invalid;}
    const auto got=f.read(reinterpret_cast<uint8_t*>(text.data),n);
    f.close();
    if(got!=n)return ReadResult::Invalid;
    text.size=n;text.data[n]=0;
    return ReadResult::Loaded;
}
bool validate(const Text& text,LayoutConfig& config){
    auto* copy=static_cast<char*>(storage_allocate(text.size+1));
    if(!copy)return false;
    memcpy(copy,text.data,text.size+1);
    const bool ok=layout_parse(copy,text.size,config);
    free(copy);return ok;
}
template<class FS> ReadResult read_valid(FS& fs,const char* path,Text& text,LayoutConfig& config){
    auto result=read_text(fs,path,text);
    if(result==ReadResult::Loaded&&!validate(text,config))return ReadResult::Invalid;
    return result;
}
template<class FS> bool remove_if_present(FS& fs,const char* path){
    return !fs.exists(path)||fs.remove(path);
}
template<class FS> bool stage(FS& fs,const char* path,const char* data,size_t len){
    if(!remove_if_present(fs,path))return false;
    auto f=fs.open(path,FILE_WRITE);
    if(!f)return false;
    const bool written=f.write(reinterpret_cast<const uint8_t*>(data),len)==len;
    f.flush();f.close();
    if(!written)return false;
    Text verify;
    return read_text(fs,path,verify)==ReadResult::Loaded&&verify.size==len&&
        memcmp(verify.data,data,len)==0;
}
bool mount_flash(LayoutStatus& state,bool initialize=false){
    // No automatic format. Only the explicit Settings confirmation may request it.
    state.flash_ready=FFat.begin(initialize,"/fs");
    return state.flash_ready;
}
bool settings_dir(){
    return FFat.exists("/Settings")||FFat.mkdir("/Settings");
}
bool persist(const char* text,size_t size,LayoutStatus& state){
    if(!settings_dir()||!stage(FFat,temp_path,text,size))return false;
    LayoutConfig previous;Text old;
    const bool primary_valid=read_valid(FFat,LAYOUT_PATH,old,previous)==ReadResult::Loaded;
    if(state.source==LayoutSource::Internal&&!primary_valid)return false;
    if(primary_valid){
        // Preserve the last known good file across a failed rename or power loss.
        if(!remove_if_present(FFat,backup_path)||!FFat.rename(LAYOUT_PATH,backup_path))return false;
    }else if(!remove_if_present(FFat,LAYOUT_PATH)){
        return false; // retain any existing good backup
    }
    if(!FFat.rename(temp_path,LAYOUT_PATH)){
        if(primary_valid&&!FFat.exists(LAYOUT_PATH)&&!FFat.rename(backup_path,LAYOUT_PATH))
            state.source=LayoutSource::Backup;
        return false;
    }
    return true;
}
void apply(){
    tools_screen_apply_layout();
    quick_controls_screen_apply_layout();
}

// Write and verify a temporary file before replacing the SD editing copy.
// If an old file exists, preserve one backup rather than truncating it.
bool export_file(const char* path,const char* temp,const char* backup,const char* data,size_t len){
    if(!SD.exists("/Settings")&&!SD.mkdir("/Settings"))return false;
    if(!stage(SD,temp,data,len))return false;
    if(SD.exists(path)){
        if(!remove_if_present(SD,backup)||!SD.rename(path,backup))return false;
    }
    if(!SD.rename(temp,path)){
        if(!SD.exists(path)&&SD.exists(backup))SD.rename(backup,path);
        return false;
    }
    return true;
}

}
const LayoutConfig& layout_config(){return current;}
LayoutStatus layout_status(){return status;}
void layout_load(){
    current=layout_defaults();status={LayoutSource::Embedded,false,false};
    if(!mount_flash(status))return;
    Text primary;
    const auto result=read_valid(FFat,LAYOUT_PATH,primary,current);
    if(result==ReadResult::Loaded){status.source=LayoutSource::Internal;return;}
    Text backup;
    const auto previous=read_valid(FFat,backup_path,backup,current);
    status.invalid_internal=result==ReadResult::Invalid||previous==ReadResult::Invalid;
    if(previous==ReadResult::Loaded)status.source=LayoutSource::Backup;
}
LayoutResult layout_save_text(const char* text,size_t length){
    if(!text||length>LAYOUT_MAX_BYTES||memchr(text,0,length))return LayoutResult::InvalidFile;
    auto* copy=static_cast<char*>(storage_allocate(length+1));
    if(!copy)return LayoutResult::WriteFailed;
    memcpy(copy,text,length);copy[length]=0;
    LayoutConfig parsed;
    const bool valid=layout_parse(copy,length,parsed);
    free(copy);
    if(!valid)return LayoutResult::InvalidFile;
    if(!mount_flash(status))return LayoutResult::FlashUnavailable;
    if(!persist(text,length,status))return LayoutResult::WriteFailed;
    current=parsed;status={LayoutSource::Internal,true,false};apply();
    return LayoutResult::Ok;
}
// Worker-only transaction. The caller reserves the layout manager until the
// result is consumed by the UI. Never touches current or LVGL from this task.
LayoutResult layout_store_download(const char* text,size_t length,bool activate,
                                   LayoutStatus& state,LayoutConfig& parsed){
    if(!text||!length||length>LAYOUT_MAX_BYTES||memchr(text,0,length))return LayoutResult::InvalidFile;
    auto* copy=static_cast<char*>(storage_allocate(length+1));
    if(!copy)return LayoutResult::WriteFailed;
    memcpy(copy,text,length);copy[length]=0;
    const bool valid=layout_parse(copy,length,parsed);
    free(copy);
    if(!valid)return LayoutResult::InvalidFile;
    StorageAccess access;
    if(!access)return LayoutResult::NoSd;
    if(!export_file(LAYOUT_PATH,"/Settings/layout_export.tmp","/Settings/layout_export.bak",text,length))
        return LayoutResult::WriteFailed;
    if(!activate)return LayoutResult::Ok;
    if(!mount_flash(state))return LayoutResult::FlashUnavailable;
    if(!persist(text,length,state))return LayoutResult::FlashWriteFailed;
    state={LayoutSource::Internal,true,false};
    return LayoutResult::Ok;
}
void layout_finish_download(LayoutResult result,bool activate,const LayoutStatus& state,const LayoutConfig& parsed){
    if(!activate)return;
    status=state;
    if(result==LayoutResult::Ok){current=parsed;apply();}
}
LayoutResult layout_import_sd(){
    StorageAccess access;
    if(!access)return LayoutResult::NoSd;
    Text text;
    const auto result=read_text(SD,LAYOUT_PATH,text);
    if(result==ReadResult::Missing)return LayoutResult::MissingFile;
    if(result!=ReadResult::Loaded)return LayoutResult::InvalidFile;
    return layout_save_text(text.data,text.size);
}
LayoutResult layout_export_sd(bool defaults){
    StorageAccess access;
    if(!access)return LayoutResult::NoSd;
    size_t len=0;const char* data=layout_embedded_data(len);
    Text active;
    if(!defaults&&status.source!=LayoutSource::Embedded){
        if(!mount_flash(status))return LayoutResult::FlashUnavailable;
        LayoutConfig parsed;
        const char* path=status.source==LayoutSource::Backup?backup_path:LAYOUT_PATH;
        if(read_valid(FFat,path,active,parsed)!=ReadResult::Loaded)return LayoutResult::InvalidFile;
        data=active.data;len=active.size;
    }
    const bool ok=defaults?
        export_file(LAYOUT_DEFAULT_PATH,"/Settings/layout_default.tmp","/Settings/layout_default.bak",data,len):
        export_file(LAYOUT_PATH,"/Settings/layout_export.tmp","/Settings/layout_export.bak",data,len);
    return ok?LayoutResult::Ok:LayoutResult::WriteFailed;
}
LayoutResult layout_restore_defaults(){
    if(!mount_flash(status))return LayoutResult::FlashUnavailable;
    // Remove only files owned by the layout manager, never the filesystem.
    if(!remove_if_present(FFat,temp_path)||!remove_if_present(FFat,backup_path)||
       !remove_if_present(FFat,LAYOUT_PATH))return LayoutResult::WriteFailed;
    current=layout_defaults();status={LayoutSource::Embedded,true,false};apply();
    return LayoutResult::Ok;
}
bool layout_initialize_flash(){
    if(!mount_flash(status)&&!mount_flash(status,true))return false;
    layout_load();apply();return status.flash_ready;
}
const char* layout_result_message(LayoutResult result){
    switch(result){
        case LayoutResult::Ok:return "Done";
        case LayoutResult::NoSd:return "SD unavailable or owned by USB";
        case LayoutResult::MissingFile:return "Missing /Settings/layout.txt on SD";
        case LayoutResult::InvalidFile:return "Invalid layout; unchanged";
        case LayoutResult::FlashUnavailable:return "Internal storage unavailable";
        case LayoutResult::WriteFailed:return "Write failed; check storage";
        case LayoutResult::FlashWriteFailed:return "Internal write failed";
    }
    return "Unknown error";
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
