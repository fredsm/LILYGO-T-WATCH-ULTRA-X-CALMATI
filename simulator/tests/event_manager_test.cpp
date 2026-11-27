#include "../../src/event_manager.h"
#include "../../src/ui_scheduler.h"
#include <atomic>
#include <thread>
#include <cstdio>
#define CHECK(e) do {if(!(e)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#e);return 1;}}while(0)
int main(){
    event_manager_init();ui_task_claim();CHECK(ui_task_is_owner());
    UiEvent e;e.kind=UiEventKind::Battery;e.value=25;e.first=true;
    CHECK(event_publish(e));e.value=75;e.first=false;CHECK(event_publish(e));
    auto s=event_stats();CHECK(s.pending==1&&s.replaced==1&&s.published==2);
    UiEvent result;CHECK(event_take(UiEventKind::Battery,result));
    CHECK(result.value==75&&!result.first&&!event_take(UiEventKind::Battery,result));
    e.kind=UiEventKind::Count;CHECK(!event_publish(e));
    CHECK(!event_take(UiEventKind::Count,result));
    event_manager_init();
    std::atomic<bool> done{false};
    // Concurrent producer and consumer; copied result must never be torn.
    std::thread worker([&]{
        if(ui_task_is_owner()) { done.store(true); return; }
        for(int i=1;i<=100000;++i){
            UiEvent t;t.kind=UiEventKind::Timezone;t.generation=uint32_t(i);
            t.value=i;t.utc.tm_year=i;t.first=(i&1)!=0;
            event_publish(t);
        }
        done.store(true);
    });
    bool valid=true;unsigned seen=0;
    do {
        if(event_take(UiEventKind::Timezone,result)){
            valid=valid&&result.generation==uint32_t(result.value)&&result.utc.tm_year==result.value&&result.first==((result.value&1)!=0);
            valid=valid&&result.generation>=seen;seen=result.generation;
        }
    }while(!done.load()||event_stats().pending);
    worker.join();CHECK(valid&&seen==100000);
    s=event_stats();CHECK(s.pending==0&&s.published==100000&&s.consumed+s.replaced==s.published);
    // Latest values for separate event kinds are independent.
    for(unsigned i=0;i<static_cast<unsigned>(UiEventKind::Count);++i){
        e.kind=static_cast<UiEventKind>(i);e.value=int(i);CHECK(event_publish(e));
    }
    CHECK(event_stats().pending==static_cast<unsigned>(UiEventKind::Count));
    for(unsigned i=0;i<static_cast<unsigned>(UiEventKind::Count);++i){
        CHECK(event_take(static_cast<UiEventKind>(i),result)&&result.value==int(i));
    }
    CHECK(event_stats().pending==0);
    // A separate input edge survives even when telemetry is overwritten.
    event_back_from_isr();event_back_from_isr();
    e.kind=UiEventKind::Clock;CHECK(event_publish(e));
    CHECK(event_take_back()&&!event_take_back());
    CHECK(event_stats().back_coalesced==1);
    CHECK(event_take(UiEventKind::Clock,result));
    // Weather payload is copied, never a reference to the worker's data.
    e.kind=UiEventKind::Weather;e.weather.temp_c=18.25f;e.weather.done=true;
    CHECK(event_publish(e));e.weather.temp_c=99;
    CHECK(event_take(UiEventKind::Weather,result)&&result.weather.temp_c==18.25f&&result.weather.done);
    // A time budget may end a slice after one job; the next slice must
    // resume at the next job and may not starve the temporarily gated ones.
    UiScheduler scheduler;
    for(unsigned i=0;i<20;++i)CHECK(scheduler.next(5,[](unsigned){return true;})==int(i%5));
    CHECK(scheduler.next(5,[](unsigned i){return i==3;})==3);
    CHECK(scheduler.next(5,[](unsigned){return false;})==-1);
    CHECK(scheduler.next(5,[](unsigned){return true;})==4);
    return 0;
}
