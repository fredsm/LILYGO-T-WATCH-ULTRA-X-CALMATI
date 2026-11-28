#include "layout_remote.h"
#include "layout_config.h"
#include "power_manager.h"
#include "radio_manager.h"
#include "storage_manager.h"
#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <atomic>
#include <cstring>
#include <cstdlib>
#ifndef TWATCH_SIMULATOR
#include <esp_http_client.h>
#include <sdkconfig.h>
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
// Arduino's WiFiClientSecure ships a different header with this same
// name. Declare the SDK function using its ESP-IDF ABI to avoid selecting
// the Arduino wrapper (which requires an externally supplied bundle).
extern "C" esp_err_t esp_crt_bundle_attach(void* conf);
#endif
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#else
#include <string>
#endif

namespace {
char source_url[LAYOUT_URL_MAX+1]{};
enum class State:uint8_t { Idle, Running, Ready };
std::atomic<State> state{State::Idle};
LayoutRemoteResult last=LayoutRemoteResult::Idle; // UI-owned
struct Request {
    char url[LAYOUT_URL_MAX+1]{};
    bool activate=false;
    LayoutStatus flash{};
    LayoutConfig parsed{};
    LayoutResult stored=LayoutResult::WriteFailed;
    LayoutRemoteResult result=LayoutRemoteResult::Idle;
} request;
#ifdef TWATCH_SIMULATOR
std::string fixture="home.power=timer\n";
int fixture_status=200;
bool fixture_complete=true;
LayoutRemoteResult fixture_error=LayoutRemoteResult::Idle;
#endif
bool https(const char* url){return strncmp(url,"https://",8)==0;}
LayoutRemoteResult download(char* buffer,size_t& size){
#ifdef TWATCH_SIMULATOR
    if(fixture_error!=LayoutRemoteResult::Idle)return fixture_error;
    if(fixture_status!=200)return LayoutRemoteResult::HttpError;
    if(fixture.size()>LAYOUT_MAX_BYTES)return LayoutRemoteResult::TooLarge;
    if(!fixture_complete||fixture.empty())return LayoutRemoteResult::Incomplete;
    size=fixture.size();memcpy(buffer,fixture.data(),size);
    return LayoutRemoteResult::Downloaded;
#else
    esp_http_client_config_t cfg{};
    cfg.url=request.url;
    cfg.timeout_ms=5000; // connect/socket inactivity
    cfg.disable_auto_redirect=true; // direct URL only; never downgrade HTTPS
    cfg.buffer_size=1024;
    if(https(request.url)){
#if CONFIG_MBEDTLS_CERTIFICATE_BUNDLE
        cfg.crt_bundle_attach=esp_crt_bundle_attach;
#else
        return LayoutRemoteResult::TlsUnavailable; // never use insecure TLS
#endif
    }
    auto client=esp_http_client_init(&cfg);
    if(!client)return LayoutRemoteResult::WorkerFailed;
    const int64_t deadline=esp_timer_get_time()+20000000;
    auto result=LayoutRemoteResult::HttpError;
    esp_http_client_set_header(client,"Accept","text/plain, */*");
    esp_http_client_set_header(client,"Accept-Encoding","identity");
    if(esp_http_client_open(client,0)==ESP_OK){
        const auto length=esp_http_client_fetch_headers(client);
        const bool chunked=esp_http_client_is_chunked_response(client);
        if(esp_http_client_get_status_code(client)==200){
            if(length>int(LAYOUT_MAX_BYTES))result=LayoutRemoteResult::TooLarge;
            else if(length<0 || (!chunked&&length==0))result=LayoutRemoteResult::Incomplete;
            else {
                result=LayoutRemoteResult::Downloaded;
                do {
                    if(radio_manager_stats().suspended){result=LayoutRemoteResult::Cancelled;break;}
                    if(esp_timer_get_time()>=deadline){result=LayoutRemoteResult::Timeout;break;}
                    // Read one extra byte to detect oversized chunked bodies.
                    const size_t remaining=LAYOUT_MAX_BYTES+1-size;
                    const int n=esp_http_client_read(client,buffer+size,int(remaining<256?remaining:256));
                    if(n<0 || (n==0&&!esp_http_client_is_complete_data_received(client))){result=LayoutRemoteResult::Incomplete;break;}
                    if(n==0)break;
                    size+=size_t(n);
                    if(size>LAYOUT_MAX_BYTES){result=LayoutRemoteResult::TooLarge;break;}
                    if(!chunked&&size==size_t(length)&&esp_http_client_is_complete_data_received(client))break;
                }while(true);
                if(result==LayoutRemoteResult::Downloaded &&
                   (!size||(!chunked&&size!=size_t(length))||
                    !esp_http_client_is_complete_data_received(client)))result=LayoutRemoteResult::Incomplete;
            }
        }
    }
    if(esp_timer_get_time()>=deadline)result=LayoutRemoteResult::Timeout;
    esp_http_client_cleanup(client);
    return result;
#endif
}
LayoutRemoteResult map(LayoutResult r,bool apply){
    switch(r){
        case LayoutResult::Ok:return apply?LayoutRemoteResult::Applied:LayoutRemoteResult::Downloaded;
        case LayoutResult::NoSd:return LayoutRemoteResult::NoSd;
        case LayoutResult::InvalidFile:return LayoutRemoteResult::InvalidLayout;
        case LayoutResult::FlashUnavailable:return LayoutRemoteResult::FlashUnavailable;
        case LayoutResult::FlashWriteFailed:return LayoutRemoteResult::FlashWriteFailed;
        default:return LayoutRemoteResult::WriteFailed;
    }
}
void run(){
    if(state.load(std::memory_order_acquire)!=State::Running)return;
    if(!radio_job_begin(RadioJob::Layout)){request.result=LayoutRemoteResult::RadioBusy;}
    else{
        if(WiFi.status()!=WL_CONNECTED)request.result=LayoutRemoteResult::NoWifi;
        else {
            auto* bytes=static_cast<char*>(storage_allocate(LAYOUT_MAX_BYTES+1));
            if(!bytes)request.result=LayoutRemoteResult::WorkerFailed;
            else {
                size_t count=0;
                request.result=download(bytes,count);
                if(request.result==LayoutRemoteResult::Downloaded){
                    if(radio_manager_stats().suspended)request.result=LayoutRemoteResult::Cancelled;
                    else {
                        request.stored=layout_store_download(bytes,count,request.activate,request.flash,request.parsed);
                        request.result=map(request.stored,request.activate);
                    }
                }
                free(bytes);
            }
        }
        // Keep radio admission until the storage transaction is finished.
        // UI consumes the result through a release/acquire mailbox.
        radio_job_end(RadioJob::Layout);
    }
    state.store(State::Ready,std::memory_order_release);
}
#ifndef TWATCH_SIMULATOR
void task(void*){run();vTaskDelete(nullptr);}
#endif
}
bool layout_remote_valid_url(const char* url){
    if(!url)return false;
    const size_t n=strlen(url);
    if(n>LAYOUT_URL_MAX)return false;
    const char* authority=https(url)?url+8:strncmp(url,"http://",7)==0?url+7:nullptr;
    if(!authority)return false;
    const char* end=strpbrk(authority,"/?");
    if(!end)end=url+n;
    if(end==authority)return false;
    for(const char* p=url;*p;++p){
        unsigned char ch=*p;
        if(ch<=32||ch>=127||ch=='#'||ch=='\\'||(p>=authority&&p<end&&ch=='@'))return false;
    }
    return true;
}
const char* layout_remote_url(){return source_url;}
bool layout_remote_set_url(const char* url){
    if(layout_remote_busy()||!url||(*url&&!layout_remote_valid_url(url)))return false;
    if(!strcmp(url,source_url))return true;
    Preferences prefs;
    if(!prefs.begin("layout_remote",false))return false;
    bool ok=*url?prefs.putString("url",url)==strlen(url):prefs.remove("url");
    prefs.end();
    if(ok)strcpy(source_url,url);
    return ok;
}
void layout_remote_load(){
    // Simulated cold boot can discard a pending result only after its worker
    // is done; native reboot naturally clears the mailbox.
    state.store(State::Idle,std::memory_order_release);last=LayoutRemoteResult::Idle;
    source_url[0]=0;
    Preferences prefs;
    if(!prefs.begin("layout_remote",true))return;
    const auto saved=prefs.getString("url","");
    if(layout_remote_valid_url(saved.c_str()))strcpy(source_url,saved.c_str());
    prefs.end();
}
bool layout_remote_busy(){return state.load(std::memory_order_acquire)!=State::Idle;}
LayoutRemoteResult layout_remote_result(){return layout_remote_busy()?LayoutRemoteResult::Pending:last;}
bool layout_remote_start(bool activate){
    if(layout_remote_busy())return false;
    if(!layout_remote_valid_url(source_url)){last=LayoutRemoteResult::InvalidUrl;return false;}
    if(power_shutting_down()){last=LayoutRemoteResult::Cancelled;return false;}
    if(WiFi.status()!=WL_CONNECTED){last=LayoutRemoteResult::NoWifi;return false;}
    if(radio_job_busy()){last=LayoutRemoteResult::RadioBusy;return false;}
    {StorageAccess sd;if(!sd){last=LayoutRemoteResult::NoSd;return false;}}
    request=Request{};strcpy(request.url,source_url);request.activate=activate;
    request.flash=layout_status();
    state.store(State::Running,std::memory_order_release);
#ifndef TWATCH_SIMULATOR
    if(xTaskCreatePinnedToCore(task,"layout_fetch",12288,nullptr,1,nullptr,0)!=pdPASS){
        state.store(State::Idle,std::memory_order_release);last=LayoutRemoteResult::WorkerFailed;return false;
    }
#endif
    return true;
}
bool layout_remote_poll(){
    if(state.load(std::memory_order_acquire)!=State::Ready)return false;
    if(power_shutting_down())return false;
    layout_finish_download(request.stored,request.activate,request.flash,request.parsed);
    last=request.result;state.store(State::Idle,std::memory_order_release);
    return true;
}
const char* layout_remote_message(){
    switch(layout_remote_result()){
        case LayoutRemoteResult::Pending:return "Downloading / validating...";
        case LayoutRemoteResult::Downloaded:return "Downloaded to SD; not applied";
        case LayoutRemoteResult::Applied:return "Downloaded, imported and applied";
        case LayoutRemoteResult::InvalidUrl:return "Enter a valid HTTP or HTTPS URL";
        case LayoutRemoteResult::NoWifi:return "Connect to Wi-Fi first";
        case LayoutRemoteResult::RadioBusy:return "Radio busy; retry later";
        case LayoutRemoteResult::NoSd:return "SD unavailable or owned by USB";
        case LayoutRemoteResult::HttpError:return "HTTP/TLS request failed (direct URL, HTTP 200 required)";
        case LayoutRemoteResult::TooLarge:return "Remote layout exceeds 4096 bytes";
        case LayoutRemoteResult::Incomplete:return "Empty or incomplete response";
        case LayoutRemoteResult::Timeout:return "Download timed out";
        case LayoutRemoteResult::TlsUnavailable:return "TLS trust bundle unavailable";
        case LayoutRemoteResult::InvalidLayout:return "Invalid layout; unchanged";
        case LayoutRemoteResult::WriteFailed:return "SD write failed; active layout unchanged";
        case LayoutRemoteResult::FlashWriteFailed:return "SD downloaded; internal write failed. Retry import.";
        case LayoutRemoteResult::FlashUnavailable:return "SD downloaded; internal storage unavailable";
        case LayoutRemoteResult::Cancelled:return "Download cancelled";
        case LayoutRemoteResult::WorkerFailed:return "Unable to start download worker";
        default:return "";
    }
}
#ifdef TWATCH_SIMULATOR
void layout_remote_sim_response(const char* body,int status,bool complete){
    fixture=body?body:"";fixture_status=status;fixture_complete=complete;fixture_error=LayoutRemoteResult::Idle;
}
void layout_remote_sim_error(LayoutRemoteResult error){fixture_error=error;}
void layout_remote_sim_pump(){if(state.load()==State::Running)run();}
#endif
