#include "mock/watch.hpp"
#include <cstdio>
#define CHECK(x) do { if (!(x)) { std::fprintf(stderr,"line %d: %s\n",__LINE__,#x); return 1; } } while(0)
int main() {
    sim::Watch w;
    CHECK(w.millis()==0 && w.battery()==100 && !w.charging());
    w.set_rtc(1000);
    CHECK(w.advance(999) && w.rtc_seconds()==1000);
    CHECK(w.advance(1) && w.rtc_seconds()==1001);
    w.set_rtc(2000);
    CHECK(w.advance(2000) && w.rtc_seconds()==2002);
    w.set_battery(-5); CHECK(w.battery()==0);
    w.set_battery(105); CHECK(w.battery()==100);
    w.set_charging(true); CHECK(w.charging());
    w.set_brightness(300); CHECK(w.brightness()==255);
    w.set_brightness(-1); CHECK(w.brightness()==0);
    w.set_display_on(false); CHECK(!w.display_on());
    CHECK(w.vibrate(50,100)); CHECK(w.haptics().size()==1);
    CHECK(w.haptics()[0].at_ms==3000 && w.haptics()[0].duration_ms==100);
    w.set_haptic_available(false); CHECK(!w.vibrate(50,100));
    CHECK(w.haptics().size()==1); w.clear_haptics(); CHECK(w.haptics().empty());
    w.button(sim::Button::power,true); w.button(sim::Button::power,true);
    w.advance(10); w.button(sim::Button::power,false);
    sim::ButtonEvent e{};
    CHECK(w.pop_button(e) && e.pressed && e.at_ms==3000);
    CHECK(w.pop_button(e) && !e.pressed && e.at_ms==3010);
    CHECK(!w.pop_button(e));
    w.set_motion(true); CHECK(w.motion());
    sim::Watch other; CHECK(!other.motion() && other.millis()==0);
    return 0;
}
