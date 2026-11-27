#include "../../src/pin_lock_screen.h"
#include "../../src/power_manager.h"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/startup.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/quick_controls_screen.h"
#include "../../src/settings_screen.h"
#include "Preferences.h"
#include <array>
#include <cstdio>
#include <cstring>

#define CHECK(expr) do { if (!(expr)) { std::fprintf(stderr, "failure line %d: %s\n", __LINE__, #expr); return 1; } } while (0)
namespace {
lv_indev_t* input;
lv_point_t point{};
bool down = false;
void pump() { for (int i=0; i<5; ++i) { lv_tick_inc(20); sim::tick(); lv_timer_handler(); } }
void sample(int x, int y, bool pressed) {
    point={x,y};down=pressed;lv_tick_inc(20);sim::dispatch_pointer(input,down);lv_timer_handler();
}
void tap(int x, int y) { sample(x,y,true);sample(x,y,false);pump(); }
lv_obj_t* find(lv_obj_t* root, const char* text) {
    if (lv_obj_check_type(root,&lv_label_class) && !std::strcmp(lv_label_get_text(root),text)) return root;
    for (unsigned i=0;i<lv_obj_get_child_count(root);++i)
        if (auto* found=find(lv_obj_get_child(root,i),text)) return found;
    return nullptr;
}
bool tap_key(const char* key) {
    auto* label=find(lv_screen_active(),key);
    if (!label) return false;
    lv_obj_update_layout(lv_screen_active());
    lv_area_t a;lv_obj_get_coords(lv_obj_get_parent(label),&a);
    tap((a.x1+a.x2)/2,(a.y1+a.y2)/2);
    return true;
}
bool persisted_lock() {
    Preferences p;p.begin("watchlock",true);bool result=p.getBool("locked",false);p.end();return result;
}
}
int main() {
    lv_init();
    auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,[](lv_indev_t*,lv_indev_data_t* data){data->point=point;data->state=down?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;});
    lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);
    auto* home=sim::create_clock();sim::initialize_screens(home);power_init();pump();
    CHECK(!pin_lock_active());
    // The boot restoration path reads the saved lock even without an SD card.
    { Preferences p;p.begin("watchlock",false);p.putBool("locked",true);p.end(); }
    pin_lock_restore();CHECK(pin_lock_active());tap(205,250);
    CHECK(pin_lock_unlock_screen_active());
    CHECK(tap_key("1") && tap_key("2") && tap_key("3") && tap_key("4"));
    CHECK(!pin_lock_active() && !persisted_lock() && lv_screen_active()==home);
    quick_controls_screen_show();pump();
    CHECK(tap_key("Lock"));
    CHECK(pin_lock_active() && persisted_lock() && lv_screen_active()==home);
    CHECK(!power_screen_locked() && !power_screen_off());
    auto* indicator = lv_obj_get_child(home, lv_obj_get_child_count(home)-2);
    CHECK(lv_obj_get_child_count(indicator)==2);
    auto* body=lv_obj_get_child(indicator,1);
    CHECK(lv_color_to_u32(lv_obj_get_style_bg_color(body,LV_PART_MAIN))==lv_color_to_u32(lv_color_hex(0x00ff80)));

    // The first touch is consumed, even at the first digit's coordinates.
    sample(85,200,true);
    CHECK(pin_lock_unlock_screen_active());
    sample(85,200,false);pump();
    CHECK(find(lv_screen_active(),"  O  O  O  O  "));
    CHECK(tap_key("9") && tap_key("8") && tap_key("7") && tap_key("6"));
    CHECK(pin_lock_active() && pin_lock_unlock_screen_active());
    CHECK(find(lv_screen_active(),"Incorrect PIN - try again"));
    CHECK(tap_key("1") && tap_key("2") && tap_key(LV_SYMBOL_BACKSPACE));
    CHECK(find(lv_screen_active(),"  *  O  O  O  "));
    CHECK(tap_key("Clear"));
    CHECK(find(lv_screen_active(),"  O  O  O  O  "));

    // Both physical buttons open/retain the keypad and never navigate.
    sim::show_home();pump();
    sim::back_button_click();pump();
    CHECK(pin_lock_unlock_screen_active());
    sim::show_home();pump();
    power_button_down(lv_tick_get());power_button_up(lv_tick_get());
    CHECK(!power_allow_click() && pin_lock_unlock_screen_active());

    // If an incidental background transition occurs, lock enforcement restores home.
    settings_screen_show();pin_lock_enforce();CHECK(lv_screen_active()==home);
    tap(205,250);CHECK(pin_lock_unlock_screen_active());
    CHECK(tap_key("1") && tap_key("2"));
    screenoff();CHECK(pin_lock_active() && lv_screen_active()==home && power_screen_off());
    power_button_down(lv_tick_get());power_button_up(lv_tick_get());
    CHECK(!power_allow_click() && !power_screen_off() && pin_lock_unlock_screen_active());
    CHECK(find(lv_screen_active(),"  O  O  O  O  "));
    CHECK(tap_key("1") && tap_key("2") && tap_key("3") && tap_key("4"));
    CHECK(!pin_lock_active() && !persisted_lock() && lv_screen_active()==home);
    CHECK(lv_color_to_u32(lv_obj_get_style_bg_color(body,LV_PART_MAIN))==lv_color_to_u32(lv_color_hex(0x333333)));
    sim::power_button_click();pump();CHECK(lv_screen_active()!=home);

    // Lock survives simulated five-second shutdown and cold boot.
    pin_lock_activate();CHECK(persisted_lock());
    power_button_down(100000);power_tick(105000);power_button_up(105000);power_tick(105000);
    CHECK(power_is_asleep() && pin_lock_active());
    power_button_down(106000);power_button_up(106100);CHECK(power_is_asleep());
    power_button_down(107000);power_tick(112000);power_button_up(112100);
    lv_tick_inc(1501);sim::tick_boot();pump();
    CHECK(!power_is_asleep() && pin_lock_active() && persisted_lock() && lv_screen_active()==home);
    tap(205,250);CHECK(pin_lock_unlock_screen_active());
    CHECK(tap_key("1") && tap_key("2") && tap_key("3") && tap_key("4"));
    CHECK(!pin_lock_active() && !persisted_lock() && lv_screen_active()==home);
    return 0;
}
