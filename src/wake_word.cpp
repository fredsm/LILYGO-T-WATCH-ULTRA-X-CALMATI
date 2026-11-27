#include "wake_word.h"
#include "audio_manager.h"
#include "radio_manager.h"
#include <atomic>
#include <esp_timer.h>
#include <LilyGoLib.h>
#include <Arduino.h>
#include <string.h>
#include <stdio.h>

// ESP-SR public headers (vendored in lib/esp-sr/).
extern "C" {
#include "esp_wn_iface.h"
#include "esp_wn_models.h"
#include "model_path.h"
}

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_heap_caps.h"
#include <WiFi.h>
#include "esp_wifi.h"
#include "esp_bt.h"
#include "esp_bt_main.h"

// libesp_audio_front_end.a calls check_chip_config() at AFE create time to
// warn if CPU/PSRAM/cache aren't set to the recommended values. It's defined
// in esp-sr's esp_process_sdkconfig.c — a file we don't compile because of
// its ~200 CONFIG_CN_SPEECH_COMMAND_IDx macros. It's a no-op if CONFIG_*
// aren't set, so shim it here. Kept even after we dropped AFE, in case some
// other .a file drags it in as a side effect during link.
extern "C" void check_chip_config(void) {}

// ---- module state ---------------------------------------------------------
//
// This engine skips the AFE (Audio Front-End) and drives the WakeNet iface
// directly. That's fine for pure wake-word detection with a single mic:
// AFE adds AEC / VAD / NS / AGC which we don't need, at a cost of ~35 KB
// contiguous internal SRAM — memory this fork can't reliably deliver at
// runtime once LVGL and the wardriver stacks are up. The direct-detect
// path fits in ~15 KB and works with whatever the mic hands us.

static std::atomic<bool> s_active{false},s_stop_req{false},s_wake_flag{false};
static std::atomic<int> s_energy{0};
static std::atomic<const char*> s_status{""};
static srmodel_list_t* s_models=nullptr;
static model_iface_data_t* s_wn_data=nullptr;
static const esp_wn_iface_t* s_wn_iface=nullptr;
static uint32_t s_token=0;

static const char *SR_PARTITION = "model";

// ---- audio task -----------------------------------------------------------

static std::atomic<int> s_dbg_phase{0};
static std::atomic<uint32_t> s_dbg_chunks{0};

static bool load_model(){
    // Do not tear down another user's live network or Bluetooth session.
    // Model allocation occurs on the worker, not the UI.
    {
        RadioTransition transition;
        if(!transition || radio_job_busy() || radio_manager_stats().suspended ||
           WiFi.getMode()!=WIFI_OFF ||
           esp_bt_controller_get_status()!=ESP_BT_CONTROLLER_STATUS_IDLE){
            s_status="radio busy";return false;
        }
    }
    s_status="loading models";
    s_models=esp_srmodel_init(SR_PARTITION);
    if(!s_models){s_status="partition 'model' missing";return false;}
    if(s_stop_req)return false;
    char* name=esp_srmodel_filter(s_models,ESP_WN_PREFIX,NULL);
    if(!name){s_status="no wakenet model in partition";return false;}
    s_wn_iface=esp_wn_handle_from_name(name);
    if(!s_wn_iface){s_status="wn iface not found";return false;}
    s_wn_data=s_wn_iface->create(name,DET_MODE_90);
    if(!s_wn_data){s_status="WN allocation failed";return false;}
    return true;
}
static void detect_loop(){
    if(s_stop_req || !load_model() || s_stop_req)return;
    const int samples=s_wn_iface->get_samp_chunksize(s_wn_data);
    if(samples<=0 || samples>16000){s_status="invalid model chunk";return;}
    const size_t bytes=size_t(samples)*sizeof(int16_t);
    auto* buf=static_cast<int16_t*>(heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
    if(!buf){s_status="feed buf alloc failed";return;}
    s_status="listening";
    size_t filled=0;
    unsigned empty=0;
    while(!s_stop_req){
        s_dbg_phase=1;
        size_t got=0;
        const auto start=esp_timer_get_time();
        bool ok=instance.mic.read(reinterpret_cast<uint8_t*>(buf)+filled,bytes-filled,&got,pdMS_TO_TICKS(100));
        audio_note_io(true,ok && got>0,uint32_t(esp_timer_get_time()-start));
        if(!got){
            if(++empty>=5){s_status="microphone timeout";break;}
            vTaskDelay(pdMS_TO_TICKS(5));continue;
        }
        empty=0;filled+=got;
        if(filled<bytes)continue;
        filled=0;
        if(s_stop_req)break;
        s_dbg_phase=2;
        uint32_t acc=0;
        for(int i=0;i<samples;++i){int v=buf[i];acc+=v<0?-v:v;}
        int level=int(acc/samples)*100/4096;s_energy=level>100?100:level;
        s_dbg_phase=3;
        auto result=s_wn_iface->detect(s_wn_data,buf);
        ++s_dbg_chunks;s_dbg_phase=4;
        if(result==WAKENET_DETECTED){s_wake_flag=true;s_status="wake!";}
        vTaskDelay(1);
    }
    heap_caps_free(buf);
}
static void audio_task(void*){
    detect_loop();
    if(s_wn_iface && s_wn_data)s_wn_iface->destroy(s_wn_data);
    s_wn_data=nullptr;s_wn_iface=nullptr;
    if(s_models)esp_srmodel_deinit(s_models);
    s_models=nullptr;s_energy=0;
    if(s_stop_req)s_status="";
    audio_release(AudioClient::WakeWord,s_token);
    s_active=false;
    vTaskDelete(nullptr);
}
bool wake_word_start(){
    bool expected=false;
    if(!s_active.compare_exchange_strong(expected,true))return true;
    s_token=audio_acquire(AudioClient::WakeWord);
    if(!s_token){s_status="microphone busy";s_active=false;return false;}
    s_stop_req=false;s_wake_flag=false;s_energy=0;s_dbg_chunks=0;s_dbg_phase=0;
    s_status="starting";
    if(xTaskCreatePinnedToCore(audio_task,"wake_word",6144,nullptr,1,nullptr,0)!=pdPASS){
        audio_release(AudioClient::WakeWord,s_token);s_status="task spawn failed";s_active=false;return false;
    }
    return true;
}
void wake_word_stop(){s_stop_req=true;}
bool wake_word_is_active(){return s_active;}
int wake_word_get_energy(){return s_active?s_energy.load():0;}
const char* wake_word_status_text(){
    if(s_active){
        static char buf[64];
        snprintf(buf,sizeof(buf),"%s p%d ch%lu",s_status.load(),s_dbg_phase.load(),(unsigned long)s_dbg_chunks.load());
        return buf;
    }
    return s_status.load();
}
bool wake_word_pending_detection(){return s_wake_flag;}
void wake_word_consume_detection(){s_wake_flag.exchange(false);}
