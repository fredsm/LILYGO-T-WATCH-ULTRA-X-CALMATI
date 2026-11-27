#include "qspi_display_pipeline.h"

namespace {
constexpr uint64_t retry_us=100000;
uint32_t duration(uint64_t a,uint64_t b){auto d=b-a;return d>UINT32_MAX?UINT32_MAX:uint32_t(d);}
void highwater(uint32_t& a,uint32_t b){if(b>a)a=b;}
}
void QspiPipeline::enable(){s_.enabled=true;}
void QspiPipeline::pause(bool p){s_.paused=p;}
void QspiPipeline::fault(bool timeout){
    if(timeout)++s_.timeouts;else ++s_.errors;
    retry_at_=now()+retry_us;
    repaint_=true;
}
bool QspiPipeline::reap(uint32_t ms){
    if(!s_.pending)return true;
    // Only one bounded wait on a failed transaction. Subsequently poll.
    if(s_.quarantined)ms=0;
    const auto start=now();
    auto result=io_.reap(io_.context,ms);
    highwater(s_.max_wait_us,duration(start,now()));
    if(result==QspiReap::Complete){
        highwater(s_.max_transfer_us,duration(queued_at_,now()));
        if(pixel_pending_)++s_.completed;
        if(s_.quarantined){++s_.recoveries;retry_at_=now()+retry_us;}
        s_.pending=s_.quarantined=false;
        return true;
    }
    if(!s_.quarantined && (ms || result==QspiReap::Error || now()-queued_at_>=uint64_t(wait_ms)*1000)){
        s_.quarantined=true;fault(result==QspiReap::Pending);
    }
    return false;
}
bool QspiPipeline::wait(){return reap(wait_ms);}
bool QspiPipeline::idle(){return reap(0);}
bool QspiPipeline::available(){return idle() && now()>=retry_at_;}
bool QspiPipeline::submit(bool pixels,uint8_t cmd,const uint8_t* data,size_t len,bool swap){
    if(s_.pending || len>capacity || (len && !data)){fault(false);return false;}
    if(!io_.queue(io_.context,pixels,cmd,data,len,swap)){fault(false);return false;}
    s_.pending=true;pixel_pending_=pixels;queued_at_=now();
    if(pixels)s_.bytes+=len;else ++s_.commands;
    return true;
}
bool QspiPipeline::command(uint8_t cmd,const uint8_t* data,size_t len){
    // Explicit panel commands (brightness/sleep/wake) may bypass cooldown,
    // but never overlap a transaction with unconfirmed ownership.
    if(!s_.enabled || !wait())return false;
    return submit(false,cmd,data,len,false) && wait();
}
bool QspiPipeline::flush(uint16_t x,uint16_t y,uint16_t w,uint16_t h,const uint8_t* data,bool swap){
    ++s_.flushes;
    const auto start=now();
    auto drop=[&](){++s_.dropped;highwater(s_.max_submit_us,duration(start,now()));return false;};
    if(!s_.enabled || s_.paused || !available())return drop();
    const uint32_t pixels=uint32_t(w)*h;
    // A legal Ultra partial area has even width, permitting aligned DMA.
    if(!w || !h || pixels>capacity/2 || (pixels&1) || !data ||
        uint32_t(x)+w>65536u || uint32_t(y)+h>65536u){fault(false);return drop();}
    uint16_t xe=x+w-1,ye=y+h-1;
    uint8_t col[]={uint8_t(x>>8),uint8_t(x),uint8_t(xe>>8),uint8_t(xe)};
    uint8_t row[]={uint8_t(y>>8),uint8_t(y),uint8_t(ye>>8),uint8_t(ye)};
    // Keep the panel's original three command windows and separate pixel CS.
    if(!command(0x2A,col,4)||!command(0x2B,row,4)||!command(0x2C,nullptr,0)
       ||!submit(true,0x2C,data,pixels*2,swap))return drop();
    highwater(s_.max_submit_us,duration(start,now()));
    return true;
}
bool QspiPipeline::service(){
    if(!idle() || now()<retry_at_ || s_.paused || !repaint_)return false;
    repaint_=false;return true;
}
QspiStats QspiPipeline::stats() const{
    auto r=s_;r.cooling=now()<retry_at_;r.active_us=s_.pending?duration(queued_at_,now()):0;return r;
}
