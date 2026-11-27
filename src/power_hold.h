#pragma once
#include <stdint.h>

// Edge-driven, wrap-safe hold detection; no blocking delay and no repeat.
class PowerHold {
public:
    static constexpr uint32_t hold_ms = 5000;
    void press(uint32_t now) { if (!down_) { down_ = true; start_ = now; } }
    bool update(uint32_t now) {
        if (!down_ || armed_ || uint32_t(now - start_) < hold_ms) return false;
        armed_ = true;
        return true;
    }
    bool release(uint32_t now) {
        if (!down_) return false;
        update(now);
        down_ = false;
        return armed_;
    }
    bool armed() const { return armed_; }
    bool down() const { return down_; }
    void reset() { down_ = armed_ = false; }
private:
    uint32_t start_ = 0;
    bool down_ = false, armed_ = false;
};
