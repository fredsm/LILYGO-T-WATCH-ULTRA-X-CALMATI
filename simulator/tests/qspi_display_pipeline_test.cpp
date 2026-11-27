#include "../../src/qspi_display_pipeline.h"
#include <cstdlib>
#include <algorithm>
#include <vector>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
struct Fake {
    uint64_t now=0;
    bool pending=false, stuck=false, reject=false, error=false, stall_command=false;
    unsigned queues=0, waits=0;
    struct Tx{bool pixels;uint8_t command;std::vector<uint8_t> bytes;};
    std::vector<Tx> history;
    static bool queue(void* c,bool pixels,uint8_t cmd,const uint8_t* data,size_t n,bool swap){
        auto& f=*static_cast<Fake*>(c);CHECK(!f.pending);
        ++f.queues;
        if(f.reject)return false;
        f.pending=true;
        Tx t{pixels,cmd,{}};
        if(n)t.bytes.assign(data,data+n);
        if(swap)for(size_t i=0;i<n;i+=2)std::swap(t.bytes[i],t.bytes[i+1]);
        f.history.push_back(t);
        if(f.stall_command && !pixels)f.stuck=true;
        return true;
    }
    static QspiReap reap(void* c,uint32_t ms){
        auto& f=*static_cast<Fake*>(c);CHECK(f.pending);
        if(ms)++f.waits;
        if(f.error)return QspiReap::Error;
        if(f.stuck){f.now+=uint64_t(ms)*1000;return QspiReap::Pending;}
        f.now+=100;f.pending=false;return QspiReap::Complete;
    }
    static uint64_t time(void* c){return static_cast<Fake*>(c)->now;}
};
int main(){
    Fake f;QspiPipeline p({&f,Fake::queue,Fake::reap,Fake::time});
    p.enable();
    uint8_t pixels[]={0x12,0x34,0x56,0x78};
    CHECK(p.flush(22,4,2,1,pixels));
    CHECK(f.history.size()==4 && f.pending);
    CHECK(f.history[0].command==0x2a && f.history[0].bytes==std::vector<uint8_t>({0,22,0,23}));
    CHECK(f.history[1].command==0x2b && f.history[1].bytes==std::vector<uint8_t>({0,4,0,4}));
    CHECK(f.history[2].command==0x2c && f.history[2].bytes.empty());
    CHECK(f.history[3].pixels && f.history[3].bytes==std::vector<uint8_t>({0x34,0x12,0x78,0x56}));
    pixels[0]=0; // LVGL may reuse its source independently of the DMA copy
    CHECK(f.history[3].bytes[1]==0x12);
    CHECK(p.wait() && p.stats().completed==1 && p.stats().bytes==4);
    // A stuck pixel transfer does not release the DMA descriptor or allow commands.
    CHECK(p.flush(22,4,2,1,pixels)); f.stuck=true;
    CHECK(!p.wait());auto s=p.stats();CHECK(s.pending && s.quarantined && s.timeouts==1);
    auto waits=f.waits, queues=f.queues;
    CHECK(!p.wait() && !p.command(0x10,nullptr,0) && !p.flush(22,4,2,1,pixels));
    CHECK(f.waits==waits && f.queues==queues && p.stats().timeouts==1);
    // Late completion reaps the old buffer; cooldown ends before repaint.
    f.stuck=false;CHECK(!p.service());CHECK(p.stats().recoveries==1);
    f.now+=100001;CHECK(p.service());CHECK(!p.service());
    p.pause(true);CHECK(!p.flush(22,4,2,1,pixels));CHECK(p.command(0x10,nullptr,0));
    p.pause(false);CHECK(p.command(0x11,nullptr,0));
    // Queue rejection is distinct from timeout; no phantom in-flight descriptor.
    f.reject=true;CHECK(!p.flush(22,4,2,1,pixels));CHECK(!p.stats().pending);
    CHECK(p.stats().errors==1);f.reject=false;f.now+=100001;CHECK(p.service());
    // Abandoning a window command must never send pixels to a stale window.
    f.stall_command=true;queues=f.queues;CHECK(!p.flush(22,4,2,1,pixels));
    CHECK(f.queues==queues+1 && p.stats().quarantined);
    f.stall_command=false;f.stuck=false;p.pause(true);CHECK(!p.service());
    f.now+=100001;CHECK(!p.service());p.pause(false);CHECK(p.service());
    // Last flush may time out with no LVGL wait callback; service still detects it.
    CHECK(p.flush(22,4,2,1,pixels));f.stuck=true;f.now+=21000;
    CHECK(!p.service() && p.stats().quarantined);
    f.stuck=false;CHECK(!p.service());f.now+=100001;CHECK(p.service());
    CHECK(!p.flush(0,0,500,500,pixels));CHECK(!p.stats().pending);
}
