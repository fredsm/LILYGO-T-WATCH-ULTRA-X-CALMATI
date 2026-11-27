#include "gps_service.h"
#include "event_manager.h"
#ifndef TWATCH_SIMULATOR
#include <LilyGoLib.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

namespace {
constexpr unsigned chunk_size=128, queue_depth=6;
struct Chunk { uint32_t generation, sequence; uint16_t len; char data[chunk_size]; };
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
StaticQueue_t queue_storage;
uint8_t queue_bytes[queue_depth*sizeof(Chunk)];
QueueHandle_t queue=nullptr;
TaskHandle_t worker=nullptr;
portMUX_TYPE fix_mux=portMUX_INITIALIZER_UNLOCKED;
Fix latest;
uint32_t generation=0, sequence=0, dropped=0;
bool enabled=false;
void publish(TinyGPSPlus& parser,uint32_t gen){
    Fix f;
    f.generation=gen;f.location=parser.location;f.date=parser.date;
    f.time=parser.time;f.satellites=parser.satellites;f.altitude=parser.altitude;
    f.speed=parser.speed;f.course=parser.course;f.hdop=parser.hdop;
    portENTER_CRITICAL(&fix_mux);latest=f;portEXIT_CRITICAL(&fix_mux);
    UiEvent e;e.kind=UiEventKind::GPS;e.generation=gen;event_publish(e);
}
void parser_task(void*){
    TinyGPSPlus parser;
    uint32_t current=0, next_sequence=0;
    Chunk chunk;
    for(;;){
        if(xQueueReceive(queue,&chunk,portMAX_DELAY)!=pdTRUE)continue;
        if(current!=chunk.generation || next_sequence!=chunk.sequence){
            // A dropped chunk or session change must not splice NMEA sentences.
            parser=TinyGPSPlus();current=chunk.generation;
        }
        next_sequence=chunk.sequence+1;
        bool complete=false;
        for(unsigned i=0;i<chunk.len;++i)if(parser.encode(chunk.data[i]))complete=true;
        if(complete)publish(parser,current);
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
    if(queue)return;
    queue=xQueueCreateStatic(queue_depth,sizeof(Chunk),queue_bytes,&queue_storage);
    if(queue && xTaskCreatePinnedToCore(parser_task,"gps_parser",3072,nullptr,2,&worker,0)!=pdPASS)
        worker=nullptr; // bounded UI fallback, never fail boot
}
void gps_service_session(bool on){
    if(!ui_task_is_owner())return;
    if(enabled==on)return;
    enabled=on;++generation;sequence=0;
    if(queue)xQueueReset(queue);
    UiEvent ignored;event_take(UiEventKind::GPS,ignored);
    if(on)invalidate();
}
void gps_service_tick(){
    if(!ui_task_is_owner()||!enabled)return;
    Chunk c{};c.generation=generation;
    while(c.len<chunk_size&&Serial1.available()>0){
        int byte=Serial1.read();if(byte<0)break;c.data[c.len++]=char(byte);
    }
    if(c.len){
        c.sequence=sequence++;
        if(worker){
            if(xQueueSend(queue,&c,0)!=pdTRUE)dropped+=c.len;
        }else for(unsigned i=0;i<c.len;++i)instance.gps.encode(c.data[i]);
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
uint32_t gps_service_dropped(){return dropped;}
#else
void gps_service_init(){}
void gps_service_session(bool){}
void gps_service_tick(){}
uint32_t gps_service_dropped(){return 0;}
#endif
