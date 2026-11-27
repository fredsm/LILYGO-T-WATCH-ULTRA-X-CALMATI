#include "gps_service.h"
#include "event_manager.h"
#ifndef TWATCH_SIMULATOR
#include <LilyGoLib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/semphr.h>

namespace {
constexpr unsigned chunk_size=128;
struct Chunk { uint32_t generation; uint16_t len; char data[chunk_size]; };
struct Fix {
    uint32_t generation=0;
    decltype(instance.gps.location) location;
    decltype(instance.gps.date) date;
    decltype(instance.gps.time) time;
    decltype(instance.gps.satellites) satellites;
    decltype(instance.gps.altitude) altitude;
    decltype(instance.gps.speed) speed;
    decltype(instance.gps.course) course;
    decltype(instance.gps.hdop) hdop;
};
StaticSemaphore_t uart_storage;
SemaphoreHandle_t uart_guard=nullptr;
TaskHandle_t worker=nullptr;
portMUX_TYPE fix_mux=portMUX_INITIALIZER_UNLOCKED;
Fix latest;
uint32_t generation=0, backlog_peak=0;
bool enabled=false;
void publish(TinyGPSPlus& parser,uint32_t gen){
    Fix f;
    f.generation=gen;f.location=parser.location;f.date=parser.date;
    f.time=parser.time;f.satellites=parser.satellites;f.altitude=parser.altitude;
    f.speed=parser.speed;f.course=parser.course;f.hdop=parser.hdop;
    portENTER_CRITICAL(&fix_mux);latest=f;portEXIT_CRITICAL(&fix_mux);
    UiEvent e;e.kind=UiEventKind::GPS;e.generation=gen;event_publish(e);
}
// Only the worker reads an active UART. The UI starts/stops the session
// around driver begin/end; the mutex covers only a nonblocking bounded read,
// never parsing, an LVGL call, a power-rail change or a wait on serial data.
void parser_task(void*){
    TinyGPSPlus parser;
    uint32_t current=0;
    for(;;){
        Chunk chunk{};
        xSemaphoreTake(uart_guard,portMAX_DELAY);
        const bool active=enabled;
        if(active){
            chunk.generation=generation;
            int available=Serial1.available();
            if(available>0 && uint32_t(available)>backlog_peak)backlog_peak=available;
            while(chunk.len<chunk_size && available-->0){
                int byte=Serial1.read();if(byte<0)break;
                chunk.data[chunk.len++]=char(byte);
            }
        }
        xSemaphoreGive(uart_guard);
        if(!active){
            ulTaskNotifyTake(pdTRUE,portMAX_DELAY);
            continue;
        }
        if(current!=chunk.generation){
            parser=TinyGPSPlus();current=chunk.generation;
        }
        bool complete=false;
        for(unsigned i=0;i<chunk.len;++i)if(parser.encode(chunk.data[i]))complete=true;
        if(complete)publish(parser,current);
        // UART 38400 baud: a 10ms interval delivers ~38 bytes. Block
        // between bounded reads so this task does not spin on an empty FIFO.
        ulTaskNotifyTake(pdTRUE,pdMS_TO_TICKS(10));
    }
}
void invalidate(){
    Fix f;
    instance.gps.location=f.location;instance.gps.date=f.date;
    instance.gps.time=f.time;instance.gps.satellites=f.satellites;
    instance.gps.altitude=f.altitude;instance.gps.speed=f.speed;
    instance.gps.course=f.course;instance.gps.hdop=f.hdop;
}
}
void gps_service_init(){
    if(uart_guard)return;
    uart_guard=xSemaphoreCreateMutexStatic(&uart_storage);
    if(uart_guard && xTaskCreatePinnedToCore(parser_task,"gps_uart",3072,nullptr,2,&worker,0)!=pdPASS)
        worker=nullptr; // bounded UI fallback, never fail boot
}
void gps_service_session(bool on){
    if(!ui_task_is_owner())return;
    if(uart_guard)xSemaphoreTake(uart_guard,portMAX_DELAY);
    const bool changed=enabled!=on;
    if(changed){enabled=on;++generation;}
    if(uart_guard)xSemaphoreGive(uart_guard);
    if(!changed)return;
    UiEvent ignored;event_take(UiEventKind::GPS,ignored);
    if(on)invalidate();
    if(worker)xTaskNotifyGive(worker);
}
void gps_service_tick(){
    if(!ui_task_is_owner()||!enabled)return;
    if(!worker){
        // Allocation failure: keep the prior bounded, single-owner path.
        int available=Serial1.available();
        if(available>0 && uint32_t(available)>backlog_peak)backlog_peak=available;
        for(unsigned n=0;n<chunk_size && available-->0;++n){
            int byte=Serial1.read();if(byte<0)break;
            instance.gps.encode(char(byte));
        }
    }
    UiEvent e;
    if(event_take(UiEventKind::GPS,e)&&e.generation==generation){
        Fix f;
        portENTER_CRITICAL(&fix_mux);f=latest;portEXIT_CRITICAL(&fix_mux);
        if(f.generation==generation){
            instance.gps.location=f.location;instance.gps.date=f.date;
            instance.gps.time=f.time;instance.gps.satellites=f.satellites;
            instance.gps.altitude=f.altitude;instance.gps.speed=f.speed;
            instance.gps.course=f.course;instance.gps.hdop=f.hdop;
        }
    }
}
uint32_t gps_service_backlog(){
    // Diagnostics must not wait on the UART mutex. The fallback value is
    // UI-owned; with a worker, report the most recent accessible snapshot.
    static uint32_t cached=0;
    if(!worker)return backlog_peak;
    if(xSemaphoreTake(uart_guard,0)==pdTRUE){
        cached=backlog_peak;xSemaphoreGive(uart_guard);
    }
    return cached;
}
#else
void gps_service_init(){}
void gps_service_session(bool){}
void gps_service_tick(){}
uint32_t gps_service_backlog(){return 0;}
#endif
