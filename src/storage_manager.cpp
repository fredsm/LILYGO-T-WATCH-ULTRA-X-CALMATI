#include "storage_manager.h"
#include <SD.h>
#include <Arduino.h>
#include <lvgl.h>
#include <src/misc/cache/instance/lv_image_cache.h>
#include <stdlib.h>
#include <string.h>
#ifdef TWATCH_SIMULATOR
#include <mutex>
#else
#include <esp_heap_caps.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#endif

namespace {
constexpr unsigned capacity=12;
constexpr size_t budget=2*1024*1024;
constexpr size_t preference_reserve=8192;
struct Job { char path[96]; void* data; size_t bytes; StorageJobFn fn; };
Job jobs[capacity]={};
unsigned head=0, count=0, local_users=0, usb_io=0;
bool media=true, suspended=false, usb=false, usb_releasing=false, active=false, remount=false;
size_t in_flight_bytes=0;
StorageStats stats;
StorageUsage usage;
#ifdef TWATCH_SIMULATOR
std::mutex mux;
using Guard=std::lock_guard<std::mutex>;
bool deferred=false, worker=true;
#else
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard { explicit Guard(portMUX_TYPE& m):m_(m){portENTER_CRITICAL(&m_);} ~Guard(){portEXIT_CRITICAL(&m_);} portMUX_TYPE& m_; };
TaskHandle_t worker=nullptr;
#endif
bool local_allowed(){return media && !suspended && !usb && !remount;}
bool idle(){return !count && !active && !local_users && !usb_io && !usb;}
void notify_worker(){
#ifndef TWATCH_SIMULATOR
    if(worker)xTaskNotifyGive(worker);
#endif
}
bool usage_job(const char*,const void*,size_t);
bool run_one(){
    Job job; bool execute;
    {
        Guard guard(mux);
        if(!count || active)return false;
        job=jobs[head];head=(head+1)%capacity;--count;active=true;
        execute=media && !usb;
    }
    uint32_t start=millis();
    bool ok=execute && job.fn(job.path,job.data,job.bytes);
    free(job.data);
    {
        Guard guard(mux);
        active=false;in_flight_bytes-=job.bytes;
        if(job.fn==usage_job)usage.pending=false;
        if(!execute)++stats.cancelled;
        else if(ok)++stats.completed;
        else ++stats.failed;
        uint32_t elapsed=millis()-start;
        if(elapsed>stats.max_job_ms)stats.max_job_ms=elapsed;
    }
    return true;
}
#ifndef TWATCH_SIMULATOR
void task(void*){
    for(;;){
        ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
        while(run_one())vTaskDelay(1);
    }
}
#endif
bool usage_job(const char*,const void*,size_t){
    uint64_t total=SD.totalBytes(),used=SD.usedBytes();
    Guard guard(mux);
    if(media && total){usage.total=total;usage.used=used;usage.valid=true;}
    return total!=0;
}
bool ensure_parent(const char* path){
    char dir[96];size_t n=strlen(path);
    if(n>=sizeof(dir))return false;
    memcpy(dir,path,n+1);
    for(char* p=dir+1;*p;++p){
        if(*p!='/')continue;
        *p=0;
        bool ok=SD.exists(dir)||SD.mkdir(dir);
        *p='/';
        if(!ok)return false;
    }
    return true;
}
bool write_file(const char* path,const void* data,size_t bytes,StorageMode mode){
    if(!ensure_parent(path))return false;
    File file=SD.open(path,mode==StorageMode::Append?FILE_APPEND:FILE_WRITE);
    if(!file)return false;
    const auto* p=static_cast<const uint8_t*>(data);
    bool ok=true;
    while(bytes){
        size_t n=bytes>4096?4096:bytes;
        if(file.write(p,n)!=n){ok=false;break;}
        bytes-=n;p+=n;
#ifndef TWATCH_SIMULATOR
        // A large file must not monopolize Core 0 or the shared SPI bus.
        if(bytes)vTaskDelay(1);
#endif
    }
    file.close();return ok;
}
bool replace_file(const char* p,const void* d,size_t n){return write_file(p,d,n,StorageMode::Replace);}
bool append_file(const char* p,const void* d,size_t n){return write_file(p,d,n,StorageMode::Append);}

// Wrap the SDK POSIX driver's file/dir open and close, retaining the original
// handle and all existing read/seek callbacks. A cached image handle also
// prevents host takeover until LVGL closes it.
decltype(lv_fs_drv_t::open_cb) original_open=nullptr;
decltype(lv_fs_drv_t::close_cb) original_close=nullptr;
decltype(lv_fs_drv_t::dir_open_cb) original_dir_open=nullptr;
decltype(lv_fs_drv_t::dir_close_cb) original_dir_close=nullptr;
void* fs_open(lv_fs_drv_t* d,const char* p,lv_fs_mode_t m){
    if(!storage_local_acquire())return nullptr;
    void* f=original_open(d,p,m);
    if(!f)storage_local_release();
    return f;
}
lv_fs_res_t fs_close(lv_fs_drv_t* d,void* f){
    auto r=original_close(d,f);storage_local_release();return r;
}
void* dir_open(lv_fs_drv_t* d,const char* p){
    if(!storage_local_acquire())return nullptr;
    void* f=original_dir_open(d,p);
    if(!f)storage_local_release();
    return f;
}
lv_fs_res_t dir_close(lv_fs_drv_t* d,void* f){
    auto r=original_dir_close(d,f);storage_local_release();return r;
}
}

void storage_init(bool present){
    {Guard guard(mux);media=present;suspended=false;}
#ifndef TWATCH_SIMULATOR
    if(!worker && xTaskCreatePinnedToCore(task,"sd_worker",4096,nullptr,1,&worker,0)!=pdPASS)worker=nullptr;
#endif
}
void* storage_allocate(size_t bytes){
    if(!bytes || bytes>budget)return nullptr;
#ifdef TWATCH_SIMULATOR
    return malloc(bytes);
#else
    if(bytes>=1024){
        void* p=heap_caps_malloc(bytes,MALLOC_CAP_SPIRAM|MALLOC_CAP_8BIT);
        if(p || bytes>4096)return p;
    }
    return malloc(bytes);
#endif
}
bool storage_submit_owned(const char* path,void* data,size_t bytes,StorageJobFn fn){
    bool ok=false;
    void* superseded=nullptr;
    {
        Guard guard(mux);
        bool valid=path && path[0]=='/' && strlen(path)<sizeof(Job::path) && data && bytes && bytes<=budget &&
                   fn && worker && local_allowed();
        // Keep two slots for settings, and merge pending replacements of
        // the same settings file (e.g. dragging the brightness slider).
        const bool preference=valid && fn==replace_file && strncmp(path,"/Settings/",10)==0;
        if(preference){
            for(unsigned i=0;i<count;++i){
                Job& j=jobs[(head+i)%capacity];
                if(j.fn==fn && strcmp(j.path,path)==0 && bytes<=budget-in_flight_bytes+j.bytes){
                    superseded=j.data;in_flight_bytes=in_flight_bytes-j.bytes+bytes;
                    j.data=data;j.bytes=bytes;++stats.accepted;++stats.coalesced;ok=true;break;
                }
            }
        }
        if(!ok && valid && count<(preference?capacity:capacity-2) &&
           in_flight_bytes+bytes <= (preference?budget:budget-preference_reserve)){
            Job& j=jobs[(head+count)%capacity];
            strcpy(j.path,path);j.data=data;j.bytes=bytes;j.fn=fn;
            ++count;in_flight_bytes+=bytes;++stats.accepted;ok=true;
        }
        if(!ok)++stats.rejected;
    }
    free(superseded);
    if(!ok){free(data);return false;}
#ifdef TWATCH_SIMULATOR
    if(!deferred)while(run_one()){}
#else
    notify_worker();
#endif
    return true;
}
bool storage_submit_owned_write(const char* path,void* data,size_t bytes,StorageMode mode){
    return storage_submit_owned(path,data,bytes,mode==StorageMode::Append?append_file:replace_file);
}
bool storage_submit_write(const char* path,const void* data,size_t bytes,StorageMode mode){
    void* copy=data?storage_allocate(bytes):nullptr;
    if(copy)memcpy(copy,data,bytes);
    return storage_submit_owned_write(path,copy,bytes,mode);
}
StorageStats storage_stats(){
    Guard guard(mux);auto s=stats;
    s.pending=count;s.active=active;s.queued_bytes=in_flight_bytes;s.local_users=local_users;
    s.media=media;s.usb_owned=usb;s.accepting=local_allowed();s.worker=bool(worker);
    return s;
}
bool storage_request_usage(){
    {
        Guard guard(mux);
        if(usage.pending)return true;
        if(!local_allowed())return false;
        usage.pending=true;
    }
    void* token=storage_allocate(1);
    if(storage_submit_owned("/",token,1,usage_job))return true;
    {Guard guard(mux);usage.pending=false;}
    return false;
}
StorageUsage storage_usage(){Guard guard(mux);return usage;}
bool storage_local_acquire(){
    Guard guard(mux);if(!local_allowed())return false;++local_users;return true;
}
void storage_local_release(){Guard guard(mux);if(local_users)--local_users;}
bool storage_try_usb_claim(){
    Guard guard(mux);
    if(!local_allowed() || !idle())return false;
    usb=true;usb_releasing=false;usage.valid=false;return true;
}
void storage_usb_release(){
    Guard guard(mux);if(!usb)return;usb_releasing=true;remount=true;if(!usb_io)usb=false;
}
bool storage_usb_acquire(){
    Guard guard(mux);
    if(!usb || usb_releasing || !media)return false;
    ++usb_io;return true;
}
void storage_usb_io_release(){
    Guard guard(mux);if(usb_io)--usb_io;
    if(!usb_io && usb_releasing)usb=false;
}
bool storage_usb_owned(){Guard guard(mux);return usb;}
bool storage_needs_remount(){Guard guard(mux);return remount;}
void storage_media_removed(){
    {Guard guard(mux);media=false;usage.valid=false;}
    notify_worker();
}
void storage_media_inserted(){Guard guard(mux);media=true;remount=false;}
bool storage_idle(){Guard guard(mux);return idle();}
void storage_suspend(){Guard guard(mux);suspended=true;}
void storage_resume(){Guard guard(mux);suspended=false;}
void storage_release_image_cache(){lv_image_cache_drop(nullptr);}
void storage_bind_lvgl(){
    lv_fs_drv_t* d=lv_fs_get_drv('A');
    if(!d || d->open_cb==fs_open)return;
    if(d->open_cb && d->close_cb){
        original_open=d->open_cb;original_close=d->close_cb;
        d->open_cb=fs_open;d->close_cb=fs_close;
    }
    if(d->dir_open_cb && d->dir_close_cb){
        original_dir_open=d->dir_open_cb;original_dir_close=d->dir_close_cb;
        d->dir_open_cb=dir_open;d->dir_close_cb=dir_close;
    }
}
#ifdef TWATCH_SIMULATOR
void storage_sim_defer(bool defer){deferred=defer;}
bool storage_sim_pump(){return run_one();}
#endif
