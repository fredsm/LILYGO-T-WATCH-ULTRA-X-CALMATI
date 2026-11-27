#include "../../src/power_manager.h"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/startup.hpp"
#include "../../src/sim/mock/hardware.hpp"
#include <array>
#include <cstdio>
#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr,"failure at %d: %s\n", __LINE__, #expr); return 1; } } while (0)
int main() {
    lv_init();
    auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    sim::initialize_screens(sim::create_clock());
    screenlock();CHECK(power_screen_locked());screenunlock();CHECK(!power_screen_locked());
    screenoff();CHECK(power_screen_off());screenon();CHECK(!power_screen_off());
    lightsleep();CHECK(power_is_asleep()&&power_screen_off());power_sim_wake();CHECK(!power_is_asleep()&&!power_screen_off()&&!power_screen_locked());
    power_button_down(10);power_tick(5009);CHECK(!power_screen_off());
    power_button_up(5009);power_tick(5009);CHECK(!power_is_asleep());
    power_button_down(6000);power_tick(11000);
    CHECK(power_screen_locked()&&power_screen_off()&&!power_is_asleep());
    CHECK(!sim::hardware.watch.display_on());
    power_button_up(11100);power_tick(11100);CHECK(power_is_asleep());
    power_tick(20000);CHECK(power_is_asleep());
    power_sim_wake();CHECK(!power_is_asleep()&&!power_screen_off()&&!power_screen_locked());
    CHECK(sim::hardware.watch.display_on());
    lv_tick_inc(1501);sim::tick_boot();CHECK(lv_screen_active()==sim::create_clock());
    power_button_down(20000);power_cancel_press();power_tick(26000);CHECK(!power_screen_locked());
    power_button_down(30000);power_button_up(35000);power_tick(35000);CHECK(power_is_asleep());
    return 0;
}
