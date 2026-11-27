#include "qspi_display.h"
#ifdef TWATCH_SIMULATOR
void qspi_display_init(){}
void qspi_display_pause(bool){}
bool qspi_display_idle(){return true;}
bool qspi_display_drain(){return true;}
void qspi_display_service(){}
bool qspi_display_prepare_sleep(){return true;}
bool qspi_display_panel_awake(){return true;}
QspiStats qspi_display_stats(){return {};}
#else
#include <Arduino.h>
#include <driver/spi_master.h>
#include <esp_attr.h>
#include <esp_timer.h>
#include <lvgl.h>
#include <string.h>

namespace {
spi_device_handle_t device=nullptr;
int chip_select=-1;
uint16_t offset_x=0,offset_y=0;
// Dedicated internal SRAM, permanently allocated; never an IDF bounce allocation.
// Only one transaction may reference this buffer/descriptor at a time.
DMA_ATTR static uint32_t staging[QspiPipeline::capacity/4];
static spi_transaction_t transaction;
bool queue(void*,bool pixels,uint8_t cmd,const uint8_t* data,size_t size,bool swap){
    if(!device || size>sizeof(staging))return false;
    memset(&transaction,0,sizeof(transaction));
    transaction.cmd=pixels?0x32:0x02;
    transaction.addr=uint32_t(cmd)<<8;
    transaction.flags=pixels?SPI_TRANS_MODE_QIO:(SPI_TRANS_MULTILINE_CMD|SPI_TRANS_MULTILINE_ADDR);
    transaction.length=size*8;
    if(size && size<=4 && !pixels){
        transaction.flags|=SPI_TRANS_USE_TXDATA;
        memcpy(transaction.tx_data,data,size);
    }else if(size){
        auto* dst=reinterpret_cast<uint8_t*>(staging);
        if(swap){
            for(size_t i=0;i<size;i+=2){dst[i]=data[i+1];dst[i+1]=data[i];}
        }else memcpy(dst,data,size);
        transaction.tx_buffer=staging;
    }
    digitalWrite(chip_select,LOW);
    const auto result=spi_device_queue_trans(device,&transaction,0);
    if(result!=ESP_OK)digitalWrite(chip_select,HIGH); // never queued
    return result==ESP_OK;
}
QspiReap reap(void*,uint32_t wait_ms){
    spi_transaction_t* result=nullptr;
    const auto err=spi_device_get_trans_result(device,&result,pdMS_TO_TICKS(wait_ms));
    if(err==ESP_ERR_TIMEOUT)return QspiReap::Pending;
    // No ownership confirmation: do not change CS, descriptor, or staging.
    if(err!=ESP_OK || result!=&transaction)return QspiReap::Error;
    digitalWrite(chip_select,HIGH);
    return QspiReap::Complete;
}
uint64_t clock_us(void*){return esp_timer_get_time();}
QspiPipeline pipeline({nullptr,queue,reap,clock_us});
bool started=false, panel_sleeping=false, panel_state_known=true;
void flush_cb(lv_display_t* disp,const lv_area_t* area,uint8_t* data){
    if(!pipeline.flush(area->x1+offset_x,area->y1+offset_y,
                        lv_area_get_width(area),lv_area_get_height(area),data))
        lv_display_flush_ready(disp);
    // With two render buffers LVGL can render the next part while DMA runs.
}
void wait_cb(lv_display_t* disp){
    pipeline.wait(); // bounded, or zero-time once quarantined
    lv_display_flush_ready(disp); // independent staging is retained until reaped
}
}
extern "C" void twatch_qspi_bind(spi_device_handle_t handle,int cs,uint16_t x,uint16_t y){
    device=handle;chip_select=cs;offset_x=x;offset_y=y;
}
extern "C" void twatch_qspi_offset(uint16_t x,uint16_t y){offset_x=x;offset_y=y;}
extern "C" bool twatch_qspi_enabled(){return started;}
extern "C" bool twatch_qspi_command(uint32_t cmd,uint8_t* data,uint32_t size){
    if(cmd==0x10 && panel_state_known && panel_sleeping)return pipeline.wait();
    if(!pipeline.command(uint8_t(cmd),data,size)){
        if(cmd==0x10 || cmd==0x11)panel_state_known=false;
        return false;
    }
    if(cmd==0x10 || cmd==0x11)panel_state_known=true;
    if(cmd==0x10)panel_sleeping=true;
    if(cmd==0x11)panel_sleeping=false;
    return true;
}
extern "C" bool twatch_qspi_end(){
    if(!pipeline.wait())return false;
    started=false;device=nullptr;return true;
}
extern "C" void twatch_qspi_push_sync(uint16_t x,uint16_t y,uint16_t w,uint16_t h,uint16_t* data){
    if(!w || uint32_t(w)*2>QspiPipeline::capacity || !pipeline.wait())return;
    uint16_t rows=QspiPipeline::capacity/(uint32_t(w)*2);
    while(h){
        const auto n=h<rows?h:rows;
        if(!pipeline.flush(x+offset_x,y+offset_y,w,n,reinterpret_cast<uint8_t*>(data),false)
           || !pipeline.wait())return;
        data+=uint32_t(w)*n;y+=n;h-=n;
    }
}
void qspi_display_init(){
#if !TWATCH_QSPI_ASYNC
    return;
#endif
    auto* disp=lv_display_get_default();
    if(!device || !disp)return;
    pipeline.enable();started=true;
    lv_display_set_flush_cb(disp,flush_cb);
    lv_display_set_flush_wait_cb(disp,wait_cb);
}
void qspi_display_pause(bool paused){pipeline.pause(paused);}
bool qspi_display_idle(){return !started || pipeline.idle();}
bool qspi_display_drain(){return !started || pipeline.wait();}
bool qspi_display_prepare_sleep(){return !started || twatch_qspi_command(0x10,nullptr,0);}
bool qspi_display_panel_awake(){return !started || (panel_state_known && !panel_sleeping && pipeline.idle());}
void qspi_display_service(){
    if(started && pipeline.service())lv_obj_invalidate(lv_screen_active());
}
QspiStats qspi_display_stats(){return pipeline.stats();}
#endif
