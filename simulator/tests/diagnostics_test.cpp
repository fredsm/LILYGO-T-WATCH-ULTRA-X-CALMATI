#include "../../src/diagnostics_manager.h"
#include "../../src/diagnostics_screen.h"
#include "../../src/task_manager.h"
#include "../../src/power_manager.h"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/mock/hardware.hpp"
#include <array>
#include <cstdio>
#include <cstring>
#define CHECK(e) do {if(!(e)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#e);return 1;}}while(0)
namespace {
lv_obj_t* find_label(lv_obj_t* p,const char* value){
    if(lv_obj_check_type(p,&lv_label_class)&&strstr(lv_label_get_text(p),value))return p;
    for(unsigned i=0;i<lv_obj_get_child_count(p);++i)
        if(auto* o=find_label(lv_obj_get_child(p,i),value))return o;
    return nullptr;
}
void step(uint32_t ms){lv_tick_inc(ms);diagnostics_sim_tick();}
void finish(){diagnostics_ui_handler();diagnostics_sim_tick();}
}
int main(){
    lv_init(); auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    sim::initialize_screens(lv_screen_active());power_init();finish();
    auto snap=diagnostics_snapshot();CHECK(snap.ready&&snap.simulated&&snap.stalls==0);
    sim::hardware.core_load[0]=22;sim::hardware.core_load[1]=87;
    CHECK(setCpuFrequencyPreference(240));step(20);finish();
    snap=diagnostics_snapshot();
    CHECK(snap.core0==22&&snap.core1==87&&snap.cpu_mhz==240);
    // An in-flight operation is visible even before a UI handler completes.
    {
        DiagScope scope(DiagStage::WardriverSD);
        step(79);CHECK(!diagnostics_snapshot().stalled);
        step(21);snap=diagnostics_snapshot();
        CHECK(snap.stalled&&snap.history_count==1&&snap.history[0].stage==DiagStage::WardriverSD);
        step(29900);CHECK(diagnostics_snapshot().history[0].duration>=30000);
    }
    finish();snap=diagnostics_snapshot();
    CHECK(!snap.stalled&&snap.history[0].recovered&&snap.max_ui_gap_ms>=30000);
    CHECK(snap.timings[static_cast<unsigned>(DiagStage::WardriverSD)].maximum==30000);
    diagnostics_scheduler_slice(3);diagnostics_scheduler_slice(4);diagnostics_scheduler_slice(17);
    snap=diagnostics_snapshot();
    CHECK(snap.slices==3&&snap.max_slice_ms==17&&snap.slice_overruns==1);
    // Intentional sleep or shutdown does not produce false stalls.
    diagnostics_pause();step(90000);diagnostics_scheduler_slice(90000);finish();CHECK(diagnostics_snapshot().stalls==1&&diagnostics_snapshot().max_slice_ms==17);
    diagnostics_resume();finish();CHECK(diagnostics_snapshot().stalls==1);
    // A stall spanning multiple operations must be attributed to the longest,
    // rather than only the first operation that happened to cross 100 ms.
    { DiagScope scope(DiagStage::Battery); step(110); }
    { DiagScope scope(DiagStage::NFC); step(900); }
    finish();CHECK(diagnostics_snapshot().history[0].stage==DiagStage::NFC);
    // Synthetic fault doesn't move watch time and captures the selected layer.
    auto before=lv_tick_get();
    diagnostics_simulate_stall(DiagStage::QspiFlush,30000);
    CHECK(lv_tick_get()==before);
    snap=diagnostics_snapshot();
    CHECK(snap.history[0].stage==DiagStage::QspiFlush&&snap.history[0].duration==30000);
    for(unsigned i=0;i<20;++i)diagnostics_simulate_stall(DiagStage::NFC,100+i);
    CHECK(diagnostics_snapshot().history_count==DIAG_HISTORY);
    task_manager_show();auto* l=find_label(lv_screen_active(),"Diagnostics");CHECK(l);
    lv_obj_send_event(lv_obj_get_parent(l),LV_EVENT_CLICKED,nullptr);
    CHECK(diagnostics_screen_is_active());
    CHECK(find_label(lv_screen_active(),"SYSTEM DIAGNOSTICS"));
    CHECK(find_label(lv_screen_active(),"Core 0: 22%"));
    CHECK(find_label(lv_screen_active(),"RECENT UI STALLS"));
    CHECK(!task_manager_is_active());
    diagnostics_reset();finish();snap=diagnostics_snapshot();CHECK(snap.ready&&snap.history_count==0&&snap.stalls==0);
    return 0;
}
