#pragma once
#include <algorithm>
#include <cstdint>
#include <deque>
#include <limits>
#include <vector>

namespace sim {
enum class Button { power, back };
struct ButtonEvent { Button button; bool pressed; uint64_t at_ms; };
struct HapticEvent { uint8_t strength; uint32_t duration_ms; uint64_t at_ms; };
class Watch {
public:
    uint64_t millis() const { return now_; }
    int64_t rtc_seconds() const { return rtc_base_ + static_cast<int64_t>((now_-rtc_set_at_)/1000); }
    void set_rtc(int64_t seconds) { rtc_base_=seconds; rtc_set_at_=now_; }
    bool advance(uint64_t ms) {
        if (ms > std::numeric_limits<uint64_t>::max()-now_) return false;
        now_ += ms; return true;
    }
    int battery() const { return battery_; }
    void set_battery(int percent) { battery_=std::clamp(percent,0,100); }
    bool charging() const { return charging_; }
    void set_charging(bool value) { charging_=value; }
    int brightness() const { return brightness_; }
    void set_brightness(int value) { brightness_=std::clamp(value,0,255); }
    uint32_t cpu_frequency() const { return cpu_mhz_; }
    bool set_cpu_frequency(uint32_t mhz) {
        if (mhz != 80 && mhz != 160 && mhz != 240) return false;
        cpu_mhz_ = mhz; return true;
    }
    bool display_on() const { return display_on_; }
    void set_display_on(bool value) { display_on_=value; }
    void set_haptic_available(bool value) { haptic_available_=value; }
    bool vibrate(uint8_t strength,uint32_t duration_ms) {
        if (!haptic_available_) return false;
        haptics_.push_back({strength,duration_ms,now_}); return true;
    }
    const std::vector<HapticEvent>& haptics() const { return haptics_; }
    void clear_haptics() { haptics_.clear(); }
    void button(Button button,bool pressed) {
        bool& state = button==Button::power ? power_ : back_;
        if (state==pressed) return;
        state=pressed; buttons_.push_back({button,pressed,now_});
    }
    bool pop_button(ButtonEvent& event) {
        if (buttons_.empty()) return false;
        event=buttons_.front(); buttons_.pop_front(); return true;
    }
    void set_motion(bool value) { motion_=value; }
    bool motion() const { return motion_; }
private:
    uint64_t now_=0,rtc_set_at_=0;
    int64_t rtc_base_=1704067200;
    int battery_=100,brightness_=255;
    uint32_t cpu_mhz_=240;
    bool charging_=false,display_on_=true,haptic_available_=true;
    bool power_=false,back_=false,motion_=false;
    std::deque<ButtonEvent> buttons_;
    std::vector<HapticEvent> haptics_;
};
}
