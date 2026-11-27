#pragma once
#include <stdint.h>

// Shared policy; only completed utilization windows are passed here.
// Upshifts are direct and immediate, downshifts require five seconds of
// sustained demand for the same lower tier.
class CpuGovernor {
public:
    static constexpr uint32_t window_ms = 500;
    static constexpr uint32_t downshift_ms = 5000;
    static uint32_t target(uint32_t core0, uint32_t core1) {
        const uint32_t load = core0 > core1 ? core0 : core1;
        return load > 80 ? 240 : load > 50 ? 160 : 80;
    }
    void reset() { candidate_ = 0; lower_ms_ = 0; }
    uint32_t evaluate(uint32_t current, uint32_t core0, uint32_t core1,
                      uint32_t elapsed_ms = window_ms) {
        const uint32_t wanted = target(core0, core1);
        if (wanted >= current) { reset(); return wanted; }
        if (wanted != candidate_) { candidate_ = wanted; lower_ms_ = 0; }
        if (elapsed_ms > window_ms * 2) { reset(); return current; }
        lower_ms_ += elapsed_ms;
        if (lower_ms_ >= downshift_ms) { reset(); return wanted; }
        return current;
    }
private:
    uint32_t candidate_ = 0, lower_ms_ = 0;
};
