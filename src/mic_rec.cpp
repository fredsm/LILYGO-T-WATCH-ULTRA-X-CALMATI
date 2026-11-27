#include "mic_rec.h"
#include "pcm_wav.h"
#include "audio_manager.h"
#include "radio_manager.h"
#include "storage_manager.h"
#include <Arduino.h>
#include <LilyGoLib.h>
#include <SD.h>
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/stream_buffer.h>
#include <atomic>
#include <string.h>
#include <math.h>

#define REC_MAX_MS      (10 * 60 * 1000)
#define SB_SIZE         (64 * 1024)
#define MIC_CHUNK       2048
#define DRAIN_TMP       4096

namespace {
StreamBufferHandle_t s_sb=nullptr;
StaticStreamBuffer_t s_sb_control;
uint8_t* s_ring=nullptr;
std::atomic<bool> s_run{false},s_task_done{true},s_recording{false},s_saved{false};
std::atomic<uint32_t> s_start_ms{0},s_session{0};
std::atomic<size_t> s_bytes{0};
std::atomic<int> s_level{0};
std::atomic<const char*> s_status{"idle"},s_capture_error{nullptr};
portMUX_TYPE path_mux=portMUX_INITIALIZER_UNLOCKED;
char s_path[48]={};
uint32_t s_audio_token=0;
bool s_voice=false;

void set_path(const char* path){
    portENTER_CRITICAL(&path_mux);strlcpy(s_path,path,sizeof(s_path));portEXIT_CRITICAL(&path_mux);
}
// Find a new filename without overwriting an earlier recording.
bool next_path(char* out,size_t sz){
    if(!SD.exists("/Recordings") && !SD.mkdir("/Recordings"))return false;
    for(int i=0;i<1000;++i){
        snprintf(out,sz,"/Recordings/rec_%03d.wav",i);
        if(!SD.exists(out))return true;
        if((i&15)==15)vTaskDelay(1);
        if(!s_run)return false;
    }
    return false;
}

static void update_level(const uint8_t *p, size_t n)
{
    const int16_t *s = (const int16_t *)p;
    size_t cnt = n / 2;
    int peak = 0;
    for (size_t i = 0; i < cnt; i++) {
        int v = s[i] < 0 ? -s[i] : s[i];
        if (v > peak) peak = v;
    }
    // Scala logaritmica (dB), come i VU-meter veri: la voce a volume normale
    // fa picchi a ~3000-8000 su 32767 (= -20/-12 dBFS), che in lineare erano
    // solo 10-24 su 100. In dB riempiono bene le barre e reagiscono alla voce.
    // Mappa la finestra utile [-52 dBFS (rumore/voce lontana) .. -8 dBFS (voce
    // forte)] su 0..100.
    int lvl;
    if (peak < 1) {
        lvl = 0;
    } else {
        float db = 20.0f * log10f((float)peak / 32767.0f);   // -90..0 dBFS
        const float floor_db = -52.0f, ceil_db = -8.0f;
        lvl = (int)((db - floor_db) / (ceil_db - floor_db) * 100.0f);
        if (lvl < 0)   lvl = 0;
        if (lvl > 100) lvl = 100;
    }
    int prior=s_level.load();
    s_level=lvl>prior?lvl:(prior>6?prior-6:0);
}


// I2S reader owns only capture and the producer side of the stream buffer.
void mic_task(void*){
    alignas(4) uint8_t buf[MIC_CHUNK];
    unsigned empty=0;
    while(s_run){
        size_t n=0;
        const auto start=esp_timer_get_time();
        bool ok=instance.mic.read(buf,sizeof(buf),&n,pdMS_TO_TICKS(100));
        audio_note_io(true,ok && n>0,uint32_t(esp_timer_get_time()-start));
        n &= ~size_t(1);
        if(n){
            empty=0;
            update_level(buf,n);
            size_t sent=xStreamBufferSend(s_sb,buf,n,pdMS_TO_TICKS(40));
            audio_note_buffer(xStreamBufferBytesAvailable(s_sb),n-sent);
            if(sent!=n){s_capture_error="SD too slow - partial WAV";s_run=false;}
        }else{
            if(++empty>=5){s_capture_error="microphone timeout";s_run=false;}
            else vTaskDelay(pdMS_TO_TICKS(5));
        }
        if(uint32_t(millis()-s_start_ms.load())>=REC_MAX_MS)s_run=false;
    }
    // Writer may free the buffer only after this release publication.
    s_task_done=true;
    vTaskDelete(nullptr);
}

void writer_task(void*){
    bool radio_owned=false;
    const char* error=nullptr;
    char path[48]={};
    {
        File file;
        if(s_voice){
            radio_owned=radio_job_begin(RadioJob::Voice);
            if(!radio_owned)error="radio busy";
            else{
                WiFi.disconnect(true,false);
                if(!WiFi.mode(WIFI_OFF))error="WiFi off failed";
            }
        }
        if(!error && s_run && !storage_stats().media)error="SD removed";
        if(!error && s_run && storage_stats().media){
            if(!next_path(path,sizeof(path)))error="no recording filename";
            else{
                file=SD.open(path,FILE_WRITE);
                if(!file)error="SD open failed";
                else{
                    uint8_t h[44];pcm_wav_header(h,0);
                    if(file.write(h,sizeof(h))!=sizeof(h))error="SD header failed";
                    else set_path(path);
                }
            }
        }
        if(!error && s_run && file){
            s_ring=static_cast<uint8_t*>(heap_caps_malloc(SB_SIZE+1,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT));
            if(s_ring)s_sb=xStreamBufferCreateStatic(SB_SIZE+1,1,s_ring,&s_sb_control);
            if(!s_sb)error="audio buffer allocation failed";
            else{
                s_task_done=false;
                s_start_ms=millis();if(s_run)s_status="recording";
                if(xTaskCreatePinnedToCore(mic_task,"mic_capture",4096,nullptr,3,nullptr,0)!=pdPASS){
                    s_task_done=true;error="capture task allocation failed";
                }
            }
        }
        if(error)s_run=false;
        // Consumer owns file and all SD operations. Keep draining even after
        // a write failure so the reader exits before its buffer is freed.
        if(s_sb){
            alignas(4) uint8_t tmp[DRAIN_TMP];
            while(!s_task_done || xStreamBufferBytesAvailable(s_sb)){
                size_t n=xStreamBufferReceive(s_sb,tmp,sizeof(tmp),pdMS_TO_TICKS(40));
                if(!storage_stats().media){error="SD removed";s_run=false;}
                if(n && !error){
                    const auto start=esp_timer_get_time();
                    size_t wrote=file.write(tmp,n);
                    audio_note_sd(uint32_t(esp_timer_get_time()-start));
                    s_bytes+=wrote & ~size_t(1);
                    if(wrote!=n){error="SD write failed - partial WAV";s_run=false;}
                }
                if(n)vTaskDelay(1);
            }
        }
        if(!error)error=s_capture_error.load();
        if(file && storage_stats().media){
            uint8_t h[44];pcm_wav_header(h,uint32_t(s_bytes.load()));
            if(!file.seek(0) || file.write(h,sizeof(h))!=sizeof(h))error="WAV finalization failed";
        }
        file.close();
    } // File and any other SDK objects destroyed before self-delete.
    if(s_sb){vStreamBufferDelete(s_sb);s_sb=nullptr;}
    free(s_ring);s_ring=nullptr;
    if(radio_owned)radio_job_end(RadioJob::Voice);
    storage_local_release();
    s_run=false;s_level=0;
    s_saved=!error && s_bytes>0;
    s_status=error?error:s_saved.load()?"saved":"no audio";
    const auto token=s_audio_token;
    // No session data is accessed after release; new start may claim it.
    audio_release(AudioClient::Recorder,token);
    s_recording=false;
    vTaskDelete(nullptr);
}

bool start(bool voice){
    bool expected=false;
    if(!s_recording.compare_exchange_strong(expected,true))return false;
    s_audio_token=audio_acquire(AudioClient::Recorder);
    if(!s_audio_token){s_status="microphone busy";s_recording=false;return false;}
    if(!storage_local_acquire()){
        audio_release(AudioClient::Recorder,s_audio_token);
        s_status="SD unavailable or busy";s_recording=false;return false;
    }
    s_voice=voice;s_saved=false;s_capture_error=nullptr;
    s_start_ms=millis();s_bytes=0;s_level=0;set_path("");
    ++s_session;s_task_done=true;s_run=true;s_status="starting";
    if(xTaskCreatePinnedToCore(writer_task,"mic_writer",8192,nullptr,1,nullptr,0)!=pdPASS){
        s_run=false;storage_local_release();audio_release(AudioClient::Recorder,s_audio_token);
        s_status="writer task allocation failed";s_recording=false;return false;
    }
    return true;
}
}

void mic_rec_start(){start(false);}
bool mic_rec_start_voice(){return start(true);}
void mic_rec_stop(){if(s_recording)s_run=false;}
void mic_rec_toggle(){if(s_recording)mic_rec_stop();else mic_rec_start();}
bool mic_rec_is_recording(){return s_recording;}
uint32_t mic_rec_elapsed_ms(){return s_recording?millis()-s_start_ms.load():0;}
size_t mic_rec_bytes(){return s_bytes;}
int mic_rec_level(){return s_level;}
bool mic_rec_saved(){return s_saved;}
uint32_t mic_rec_session(){return s_session;}
bool mic_rec_copy_path(char* out,size_t size){
    if(!out || !size)return false;
    portENTER_CRITICAL(&path_mux);strlcpy(out,s_path,size);portEXIT_CRITICAL(&path_mux);
    return out[0]!=0;
}
const char* mic_rec_last_path(){
    // Existing UI-only accessor; worker consumers use the copy API.
    static char path[48];mic_rec_copy_path(path,sizeof(path));return path;
}
const char* mic_rec_status_text(){return s_recording && !s_run?"saving":s_status.load();}
