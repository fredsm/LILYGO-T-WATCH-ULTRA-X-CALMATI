#include "../../src/power_manager.h"
#include "../../src/settings_screen.h"
#include "../../src/pin_lock_screen.h"
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
lv_obj_t* find_dropdown(lv_obj_t* root) {
    if (lv_obj_check_type(root,&lv_label_class) && !std::strcmp(lv_label_get_text(root),"CPU Frequency")) {
        auto* row=lv_obj_get_parent(root);
        for (unsigned i=0;i<lv_obj_get_child_count(row);++i) {
            auto* child=lv_obj_get_child(row,i);
            if (lv_obj_check_type(child,&lv_dropdown_class)) return child;
        }
    }
    for (unsigned i=0;i<lv_obj_get_child_count(root);++i)
        if (auto* result=find_dropdown(lv_obj_get_child(root,i))) return result;
    return nullptr;
}
uint32_t saved() {
    Preferences prefs;prefs.begin("watchpower",true);
    auto mhz=prefs.getUInt("cpu_mhz",CPU_DEFAULT_MHZ);prefs.end();return mhz;
}
void select(lv_obj_t* dropdown, uint32_t index) {
    lv_dropdown_set_selected(dropdown,index);
    lv_obj_send_event(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
}
void reboot(uint32_t now) {
    deepsleep();
    power_button_down(now);power_tick(now+5000);power_button_up(now+5100);
    lv_tick_inc(1501);sim::tick_boot();
}
}
int main() {
    lv_init();
    auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    sim::initialize_screens(sim::create_clock());power_init();
    CHECK(getCpuFrequency()==240 && getPreferredCpuFrequency()==240 && saved()==240);
    CHECK(increaseCpuFrequency() && getCpuFrequency()==240);
    CHECK(decreaseCpuFrequency() && getCpuFrequency()==160);
    CHECK(decreaseCpuFrequency() && getCpuFrequency()==80);
    CHECK(decreaseCpuFrequency() && getCpuFrequency()==80);
    CHECK(increaseCpuFrequency() && getCpuFrequency()==160);
    CHECK(increaseCpuFrequency() && getCpuFrequency()==240);
    CHECK(setCpuFrequency(80) && getCpuFrequency()==80);
    CHECK(!setCpuFrequency(100) && !setCpuFrequencyPreference(0) && getCpuFrequency()==80);
    CHECK(saved()==240 && getPreferredCpuFrequency()==240);
    CHECK(restoreCpuFrequency() && getCpuFrequency()==240);

    // Settings affects real simulated CPU state and survives without a card.
    sim::hardware.sd_ready=false;
    settings_screen_show();
    auto* dropdown=find_dropdown(lv_screen_active());
    CHECK(dropdown && lv_dropdown_get_selected(dropdown)==2);
    CHECK(!std::strcmp(lv_dropdown_get_options(dropdown),"80 MHz\n160 MHz\n240 MHz"));
    select(dropdown,0);
    CHECK(getCpuFrequency()==80 && sim::hardware.watch.cpu_frequency()==80 && saved()==80);
    select(dropdown,1);
    CHECK(getCpuFrequency()==160 && getPreferredCpuFrequency()==160 && saved()==160);
    select(dropdown,2);
    CHECK(getCpuFrequency()==240 && saved()==240);
    select(dropdown,1);
    CHECK(getCpuFrequency()==160 && saved()==160);
    CHECK(setCpuFrequency(80) && saved()==160 && getPreferredCpuFrequency()==160);
    screenoff();screenon();
    CHECK(getCpuFrequency()==80);
    // Manual operations or sleep transitions never select another policy automatically.
    pin_lock_activate();CHECK(getCpuFrequency()==80);
    lightsleep();CHECK(power_is_asleep());
    power_button_down(10000);power_button_up(10100);
    CHECK(!power_is_asleep() && getCpuFrequency()==80);
    CHECK(restoreCpuFrequency() && getCpuFrequency()==160);
    CHECK(setCpuFrequency(240) && saved()==160);

    reboot(20000);
    CHECK(!power_is_asleep() && getCpuFrequency()==160 && getPreferredCpuFrequency()==160);
    // An invalid stored setting must never be sent to the clock driver.
    { Preferences p;p.begin("watchpower",false);p.putUInt("cpu_mhz",999);p.end(); }
    reboot(30000);
    CHECK(!power_is_asleep() && getCpuFrequency()==240 && getPreferredCpuFrequency()==240);
    return 0;
}
