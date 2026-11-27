#include "../../src/power_manager.h"
#include "../../src/audio_manager.h"
#include "../../src/storage_manager.h"
#include "../../src/radio_manager.h"
#include "../../src/alarm.h"
#include "../../src/settings_screen.h"
#include "SD.h"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/startup.hpp"
#include "../../src/sim/mock/hardware.hpp"
#include <array>
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"failure at %d: %s\n", __LINE__, #expr); return 1; } } while (0)
static lv_obj_t* find_timer(lv_obj_t* root) {
    if (lv_obj_check_type(root,&lv_label_class) &&
        std::strcmp(lv_label_get_text(root),"ScreenOff Timer")==0) {
        auto* row=lv_obj_get_parent(root);
        for (uint32_t i=0;i<lv_obj_get_child_count(row);++i) {
            auto* child=lv_obj_get_child(row,i);
            if (lv_obj_check_type(child,&lv_dropdown_class)) return child;
        }
    }
    for (uint32_t i=0;i<lv_obj_get_child_count(root);++i)
        if (auto* result=find_timer(lv_obj_get_child(root,i))) return result;
    return nullptr;
}
int main() {
    lv_init();
    auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    sim::initialize_screens(sim::create_clock());
    power_init();
    CHECK(power_get_screenoff_timeout()==60000);
    // Legacy/missing setting stays at the default. Explicit saved values reload.
    SD.mkdir("/Settings");
    { auto f=SD.open("/Settings/settings.txt",FILE_WRITE);f.print("dim_timeout_idx=0\n"); }
    settings_screen_load();CHECK(power_get_screenoff_timeout()==60000);
    { auto f=SD.open("/Settings/settings.txt",FILE_WRITE);f.print("screenoff_timeout_idx=2\n"); }
    settings_screen_load();CHECK(power_get_screenoff_timeout()==3000000);
    { auto f=SD.open("/Settings/settings.txt",FILE_WRITE);f.print("screenoff_timeout_idx=0\n"); }
    settings_screen_load();CHECK(power_get_screenoff_timeout()==0);
    { auto f=SD.open("/Settings/settings.txt",FILE_WRITE);f.print("screenoff_timeout_idx=99\n"); }
    settings_screen_load();CHECK(power_get_screenoff_timeout()==60000);
    settings_screen_show();
    auto* dropdown=find_timer(lv_screen_active());
    CHECK(dropdown && lv_dropdown_get_selected(dropdown)==1);
    lv_dropdown_set_selected(dropdown,2);
    lv_obj_send_event(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
    CHECK(power_get_screenoff_timeout()==3000000);
    CHECK(sim_sd_files.at("/Settings/settings.txt")->find("screenoff_timeout_idx=2\n")!=std::string::npos);
    lv_dropdown_set_selected(dropdown,1);
    lv_obj_send_event(dropdown,LV_EVENT_VALUE_CHANGED,nullptr);
    CHECK(power_get_screenoff_timeout()==60000);
    sim::show_home();
    screenlock();CHECK(power_screen_locked());screenunlock();CHECK(!power_screen_locked());
    screenoff();CHECK(power_screen_off());screenon();CHECK(!power_screen_off());
    power_note_activity(100);
    power_tick(60099);CHECK(!power_screen_off());
    power_tick(60100);CHECK(power_screen_off()&&!power_is_asleep());
    power_button_down(60101);CHECK(!power_screen_off());
    power_button_up(60102);CHECK(!power_allow_click());
    power_tick(120101);CHECK(!power_screen_off());
    power_tick(120102);CHECK(power_screen_off());
    screenon();
    power_set_screenoff_timeout(0,0);power_tick(4000000);CHECK(!power_screen_off());
    power_set_screenoff_timeout(3000000,0xfffffff0u);
    power_tick(0xfffffff0u+2999999u);CHECK(!power_screen_off());
    power_tick(0xfffffff0u+3000000u);CHECK(power_screen_off());
    screenon();power_set_screenoff_timeout(60000,0);
    lightsleep();CHECK(power_is_asleep()&&power_screen_off());
    power_button_down(1);CHECK(!power_is_asleep()&&!power_screen_off()&&!power_screen_locked());
    power_button_up(2);CHECK(!power_allow_click());
    power_button_down(10);power_tick(5009);CHECK(!power_screen_off());
    power_button_up(5009);power_tick(5009);CHECK(!power_is_asleep());CHECK(power_allow_click());
    power_button_down(6000);power_tick(11000);
    CHECK(power_screen_locked()&&power_screen_off()&&!power_is_asleep());
    CHECK(!sim::hardware.watch.display_on());
    power_button_up(11100);power_tick(11100);CHECK(power_is_asleep());
    power_tick(20000);CHECK(power_is_asleep());
    power_button_down(20100);power_button_up(20101);CHECK(power_is_asleep());CHECK(!power_allow_click());
    power_tick(26000);CHECK(power_is_asleep());
    power_button_down(27000);power_tick(31999);CHECK(power_is_asleep());
    power_tick(32000);CHECK(!power_is_asleep()&&!power_screen_off()&&!power_screen_locked());
    power_button_up(32100);CHECK(!power_allow_click());
    CHECK(sim::hardware.watch.display_on());
    lv_tick_inc(1501);sim::tick_boot();CHECK(lv_screen_active()==sim::create_clock());
    power_button_down(33000);power_cancel_press();power_tick(39000);CHECK(!power_screen_locked());
    // An enabled alarm must keep the CPU available while the screen is off.
    alarm_set(7,0,true);
    screenoff();lightsleep();
    CHECK(!power_is_asleep() && power_lifecycle_stats().last_idle_blockers&BlockDeadline);
    screenon();alarm_set(7,0,false);
    auto token=audio_acquire(AudioClient::Recorder);CHECK(token);
    CHECK(storage_local_acquire());
    power_button_down(40000);power_button_up(45000);power_tick(45000);
    CHECK(!power_is_asleep() && power_lifecycle_stats().stage==PowerStage::Stopping);
    CHECK(power_lifecycle_stats().blockers&BlockAudio);
    CHECK(!audio_acquire(AudioClient::Chime));
    audio_release(AudioClient::Recorder,token);
    power_tick(45010);CHECK(!power_is_asleep() && power_lifecycle_stats().stage==PowerStage::Draining);
    CHECK(!storage_local_acquire());
    storage_local_release();power_tick(45020);CHECK(power_is_asleep());
    CHECK(power_lifecycle_stats().last_shutdown_blockers&(BlockAudio|BlockStorage));
    return 0;
}
