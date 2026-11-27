#include "../../src/event_manager.h"
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
    CHECK(event_stats().pending==4);
    for(unsigned i=0;i<4;++i){
        CHECK(event_take(static_cast<UiEventKind>(i),result)&&result.value==int(i));
    }
    CHECK(event_stats().pending==0);
    return 0;
}
