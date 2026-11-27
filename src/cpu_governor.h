#pragma once
#include <stdint.h>

struct CpuGovernorStats {
    bool running=false; // populated by the power manager snapshot
    uint32_t valid_windows=0, invalid_windows=0, consecutive_invalid=0;
    uint32_t guarded_windows=0, upshifts=0, downshifts=0, failsafe_upshifts=0, failed_changes=0;
    uint32_t raw_target=0, safe_target=0, candidate=0, lower_ms=0;
    uint32_t max_change_ms=0, sampled_ms[3]={};
};

// Raw demand thresholds remain exact. A lower clock increases occupancy
// for the same cycle demand, so downshifts must also fit at the lower tier.
class CpuGovernor {
public:
    static constexpr uint32_t window_ms = 500;
    static constexpr uint32_t min_window_ms = 400;
    static constexpr uint32_t max_window_ms = 1000;
    static constexpr uint32_t downshift_ms = 5000;
    static constexpr uint32_t invalid_limit = 2;
    static uint32_t target(uint32_t core0, uint32_t core1) {
        const uint32_t load = core0 > core1 ? core0 : core1;
        return load > 80 ? 240 : load > 50 ? 160 : 80;
    }
    static uint32_t safe_target(uint32_t current, uint32_t core0, uint32_t core1) {
        const uint32_t raw = target(core0, core1);
        if (raw >= current) return raw;
        const uint32_t load = core0 > core1 ? core0 : core1;
        // Five points headroom below the existing upward thresholds.
        // Cross-multiply to avoid rounding a marginal workload downward.
        if (raw == 80 && load * current <= 45u * 80u) return 80;
        if (current > 160 && load * current <= 75u * 160u) return 160;
        return current;
    }
    void reset() {
        clear_dwell();
        stats_.consecutive_invalid=0;
        stats_.raw_target=stats_.safe_target=0;
    }
    void clear() { stats_={}; }
    uint32_t evaluate(uint32_t current, uint32_t core0, uint32_t core1,
                      uint32_t elapsed_ms = window_ms) {
        if (elapsed_ms < min_window_ms || elapsed_ms > max_window_ms ||
            core0 > 100 || core1 > 100) return invalid(current);
        ++stats_.valid_windows;
        stats_.consecutive_invalid=0;
        stats_.sampled_ms[current==80?0:current==160?1:2]+=elapsed_ms;
        const uint32_t raw=target(core0,core1);
        const uint32_t wanted=safe_target(current,core0,core1);
        stats_.raw_target=raw;stats_.safe_target=wanted;
        if(wanted!=raw)++stats_.guarded_windows;
        if (wanted >= current) { clear_dwell(); return wanted; }
        if (wanted != stats_.candidate) {
            clear_dwell();stats_.candidate=wanted;
        }
        stats_.lower_ms += elapsed_ms;
        if (stats_.lower_ms >= downshift_ms) { clear_dwell(); return wanted; }
        return current;
    }
    uint32_t invalid(uint32_t current) {
        clear_dwell();
        ++stats_.invalid_windows;
        if(stats_.consecutive_invalid<invalid_limit)++stats_.consecutive_invalid;
        stats_.raw_target=0;
        stats_.safe_target=stats_.consecutive_invalid>=invalid_limit?240:current;
        return stats_.safe_target;
    }
    void record_change(uint32_t from,uint32_t to,bool ok,uint32_t elapsed_ms) {
        if(elapsed_ms>stats_.max_change_ms)stats_.max_change_ms=elapsed_ms;
        if(!ok){++stats_.failed_changes;clear_dwell();return;}
        if(to>from){
            ++stats_.upshifts;
            if(stats_.consecutive_invalid>=invalid_limit)++stats_.failsafe_upshifts;
        }else if(to<from)++stats_.downshifts;
    }
    CpuGovernorStats stats() const { return stats_; }
private:
    void clear_dwell(){stats_.candidate=stats_.lower_ms=0;}
    CpuGovernorStats stats_{};
};
