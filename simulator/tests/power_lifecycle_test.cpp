#include "../../src/power_lifecycle.h"
#include <cstdlib>
#define CHECK(x) do{if(!(x))std::abort();}while(0)
int main(){
    PowerLifecycle p;
    p.screen(true,UINT32_MAX-999);
    CHECK(!p.idle_due(3999) && p.idle_due(4000));
    p.idle_blocked(BlockDeadline|BlockRadio);
    CHECK(p.snapshot(4000).last_idle_blockers==(BlockDeadline|BlockRadio));
    p.light_enter(4001);p.light_exit(5010,true);
    CHECK(p.snapshot(5010).light_entries==1 && p.snapshot(5010).light_wakes==1);
    CHECK(p.snapshot(5010).last_light_ms==1009);
    p.light_enter(6010);p.light_exit(6011,false);
    CHECK(p.snapshot(6011).light_aborts==1);
    p.screen(false,7000);CHECK(p.snapshot(7000).stage==PowerStage::Awake);
    p.begin_shutdown(UINT32_MAX-100);
    p.screen(false,0); // no cancellation once shutdown has been requested
    CHECK(!p.advance(BlockAudio|BlockRadio,0));
    CHECK(p.snapshot(0).stage==PowerStage::Stopping);
    CHECK(p.advance(0,100));
    CHECK(!p.advance(BlockStorage,200));
    CHECK(p.advance(0,300));
    CHECK(!p.advance(BlockDisplay|BlockSpi,400));
    CHECK(p.advance(0,500));
    CHECK(!p.advance(BlockI2c,600));
    CHECK(!p.advance(BlockDriver,700));
    CHECK(p.advance(0,800));
    auto s=p.snapshot(900);
    CHECK(s.stage==PowerStage::DeepSleep && s.shutdown_ms==901);
    CHECK(s.shutdown_blockers==(BlockAudio|BlockRadio|BlockStorage|BlockDisplay|BlockSpi|BlockI2c|BlockDriver));
    CHECK(!p.advance(0,1000));
    p.reboot();CHECK(p.snapshot(1000).stage==PowerStage::Awake);
}
