#include "../../src/cpu_governor.h"
#include "../../src/power_manager.h"
#include "../../src/settings_screen.h"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/startup.hpp"
#include "../../src/sim/mock/hardware.hpp"
#include "Preferences.h"
#include <array>
#include <cstdio>
#include <cstring>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"failure line %d: %s\n",__LINE__,#expr);return 1; } } while(0)
namespace {
void windows(unsigned n) {
    while (n--) { lv_tick_inc(CpuGovernor::window_ms);lv_timer_handler(); }
}
void load(unsigned a,unsigned b) { sim::hardware.core_load[0]=a;sim::hardware.core_load[1]=b; }
lv_obj_t* find(lv_obj_t* root,const lv_obj_class_t* type,const char* text=nullptr) {
    if (lv_obj_check_type(root,type)&&(!text||!std::strcmp(lv_label_get_text(root),text))) return root;
    for (unsigned i=0;i<lv_obj_get_child_count(root);++i)
        if (auto* result=find(lv_obj_get_child(root,i),type,text)) return result;
    return nullptr;
}
uint32_t saved() { Preferences p;p.begin("watchpower",true);auto v=p.getUInt("cpu_mhz",240);p.end();return v; }
}
int main() {
    CHECK(CpuGovernor::target(0,0)==80);
    CHECK(CpuGovernor::target(50,30)==80);
    CHECK(CpuGovernor::target(1,51)==160);
    CHECK(CpuGovernor::target(80,0)==160);
    CHECK(CpuGovernor::target(0,81)==240);
    CHECK(CpuGovernor::target(100,1)==240);
    CpuGovernor policy;
    CHECK(policy.evaluate(80,10,90)==240);
    CHECK(policy.evaluate(80,51,5)==160);
    for (int i=0;i<9;++i) CHECK(policy.evaluate(240,1,1)==240);
    CHECK(policy.evaluate(240,1,1)==80);
    for (int i=0;i<9;++i) CHECK(policy.evaluate(240,45,5)==240);
    CHECK(policy.evaluate(240,1,1)==240); // different lower tier resets dwell
    CHECK(policy.evaluate(240,90,1)==240);
    for (int i=0;i<9;++i) CHECK(policy.evaluate(240,30,5)==240);
    CHECK(policy.evaluate(240,30,5,2000)==240); // stale window cannot downshift

    lv_init();auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    sim::initialize_screens(sim::create_clock());power_init();
    CHECK(!cpuAutomaticEnabled() && getCpuFrequency()==240 && getCpuLoad(0)==-1);
    settings_screen_show();
    auto* label=find(lv_screen_active(),&lv_label_class,"CPU Frequency");
    CHECK(label);
    auto* dropdown=find(lv_obj_get_parent(label),&lv_dropdown_class);
    CHECK(dropdown&&lv_dropdown_get_selected(dropdown)==2);
    lv_dropdown_set_selected(dropdown,3);lv_obj_send_event(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
    CHECK(cpuAutomaticEnabled() && saved()==CPU_AUTO && getCpuFrequency()==240);
    load(10,15);windows(9);CHECK(getCpuFrequency()==240);
    windows(1);CHECK(getCpuFrequency()==80 && getCpuLoad(0)==10 && getCpuLoad(1)==15);
    load(51,2);windows(1);CHECK(getCpuFrequency()==160);
    load(0,81);windows(1);CHECK(getCpuFrequency()==240);
    load(80,0);windows(10);CHECK(getCpuFrequency()==240); // unsafe projected demand
    load(50,0);windows(9);CHECK(getCpuFrequency()==240);
    windows(1);CHECK(getCpuFrequency()==160);
    load(0,25);windows(10);CHECK(getCpuFrequency()==160); // 50% at 80 MHz leaves no margin
    load(0,22);windows(10);CHECK(getCpuFrequency()==80);
    load(100,0);windows(1);CHECK(getCpuFrequency()==240); // direct 80->240
    load(0,0);windows(10);CHECK(getCpuFrequency()==80); // direct 240->80
    // Delayed timer windows do not count as idle; two invalid windows fail high.
    lv_tick_inc(2000);lv_timer_handler();CHECK(getCpuFrequency()==80 && getCpuLoad(0)==-1);
    lv_tick_inc(2000);lv_timer_handler();CHECK(getCpuFrequency()==240&&cpu_governor_stats().failsafe_upshifts==1);
    load(0,0);windows(10);CHECK(getCpuFrequency()==80);
    // Transient HAL error leaves clock unchanged and is visible; next high window retries.
    sim::hardware.watch.fail_next_cpu_change();
    load(100,0);windows(1);CHECK(getCpuFrequency()==80&&cpu_governor_stats().failed_changes==1);
    windows(1);CHECK(getCpuFrequency()==240);
    load(0,0);windows(10);CHECK(getCpuFrequency()==80);
    screenoff();load(100,0);windows(1);CHECK(getCpuFrequency()==240); // background demand
    screenon();

    // Fixed mode stops automatic changes; reverting to Auto restarts fresh.
    CHECK(setCpuFrequencyPreference(80));load(99,99);windows(12);
    CHECK(getCpuFrequency()==80 && !cpuAutomaticEnabled() && getCpuLoad(1)==-1 && saved()==80);
    CHECK(setCpuFrequencyPreference(CPU_AUTO));
    load(0,0);windows(10);CHECK(getCpuFrequency()==80);
    lightsleep();CHECK(power_is_asleep() && getCpuLoad(0)==-1);
    load(100,100);windows(12);CHECK(getCpuFrequency()==80);
    power_button_down(10000);power_button_up(10010);
    CHECK(!power_is_asleep()&&getCpuFrequency()==240&&cpuAutomaticEnabled());
    load(0,0);windows(10);CHECK(getCpuFrequency()==80);
    deepsleep();CHECK(power_is_asleep());
    power_button_down(20000);power_tick(25000);power_button_up(25100);
    lv_tick_inc(1501);sim::tick_boot();
    CHECK(!power_is_asleep()&&cpuAutomaticEnabled()&&saved()==CPU_AUTO);
    CHECK(getCpuFrequency()==240);
    CHECK(cpu_governor_stats().valid_windows==0);
    load(15,15);windows(1);CHECK(getCpuFrequency()==240&&getCpuLoad(0)==-1); // boot animation skipped timer windows
    windows(10);CHECK(getCpuFrequency()==80);
    CHECK(setCpuFrequencyPreference(240));load(0,0);windows(12);CHECK(getCpuFrequency()==240);
    return 0;
}
