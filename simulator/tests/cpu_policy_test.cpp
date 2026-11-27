#include "../../src/cpu_governor.h"
#include "../../src/cpu_load_sampler.h"
#include <cstdio>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(){
    // Exact raw thresholds and either-core pressure, including direct upshift.
    CHECK(CpuGovernor::target(50,0)==80);
    CHECK(CpuGovernor::target(51,0)==160);
    CHECK(CpuGovernor::target(80,0)==160);
    CHECK(CpuGovernor::target(0,81)==240);
    CHECK(CpuGovernor::safe_target(80,0,81)==240);
    // A 48 MHz-equivalent demand: 60% at 80, 30% at 160, 20% at 240.
    // It must settle at 160, rather than cycling 80 <-> 160.
    CpuGovernor g;
    uint32_t current=240;
    for(unsigned i=0;i<40;++i){
        auto next=g.evaluate(current,0,4800/current);
        if(next!=current)g.record_change(current,next,true,1);
        current=next;
    }
    CHECK(current==160&&g.stats().downshifts==1&&g.stats().upshifts==0);
    CHECK(g.stats().guarded_windows==40);
    // Below 45% at 80: one-step 240 -> 80 is still allowed.
    g.reset();
    for(unsigned i=0;i<9;++i)CHECK(g.evaluate(240,15,0)==240);
    CHECK(g.evaluate(240,15,0)==80);
    // A changed workload or stale window breaks dwell.
    for(unsigned i=0;i<9;++i)CHECK(g.evaluate(240,15,0)==240);
    CHECK(g.evaluate(240,16,0)==240);
    CHECK(g.evaluate(240,15,0)==240);
    CHECK(g.evaluate(240,15,0,1001)==240);
    CHECK(g.stats().lower_ms==0);
    CHECK(g.evaluate(80,101,0)==80); // second invalid window: hold, never upshift
    for(uint32_t tier=80;tier<=240;tier+=80){
        for(unsigned i=0;i<20;++i)CHECK(g.invalid(tier)==tier);
        CHECK(g.stats().safe_target==tier&&g.stats().lower_ms==0);
    }
    CHECK(g.stats().consecutive_invalid==62);
    CHECK(g.evaluate(80,0,81)==240&&g.stats().consecutive_invalid==0);
    g.record_change(80,240,true,3);
    CHECK(g.stats().upshifts==1&&g.stats().max_change_ms==3);
    // Verify tick coverage, exact boundaries, integer rounding and wrapping.
    CHECK(cpu_window_load({0,0},{500,250},500,1000)==50);
    CHECK(cpu_window_load({0,0},{500,251},500,1000)==51);
    CHECK(cpu_window_load({0,0},{500,400},500,1000)==80);
    CHECK(cpu_window_load({0,0},{500,401},500,1000)==81);
    CHECK(cpu_window_load({UINT32_MAX-99,UINT32_MAX-199},{400,50},500,1000)==50);
    CHECK(cpu_window_load({0,0},{399,10},500,1000)==-1);
    CHECK(cpu_window_load({0,0},{601,10},500,1000)==-1);
    CHECK(cpu_window_load({0,0},{500,501},500,1000)==-1);
    CHECK(cpu_window_load({0,0},{100,90},100,1000)==-1);
    CHECK(cpu_window_load({0,0},{0,0},500,1000)==-1);
    CHECK(cpu_window_load({0,0},{1100,10},1100,1000)==-1);
    CHECK(cpu_window_load({0,0},{500,0},500,0)==-1);
    return 0;
}
