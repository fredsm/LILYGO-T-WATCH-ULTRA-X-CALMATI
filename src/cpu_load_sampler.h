#pragma once
#include <stdint.h>

// Shared per-core scheduler samples. Independent consumers keep their own
// baselines; diagnostics and the governor do not reset each other's counters.
struct CpuSamples { uint32_t total, busy; };
bool cpu_sampler_begin();
CpuSamples cpu_sampler_snapshot(unsigned core);

// Shared validation for both consumers; unsigned deltas tolerate counter wrap.
// Tick occupancy is approximate, not ESP hardware performance-counter data.
inline int cpu_window_load(CpuSamples prev,CpuSamples next,uint32_t elapsed_ms,uint32_t tick_hz) {
    if(elapsed_ms<400 || elapsed_ms>1000 || !tick_hz)return -1;
    const uint32_t total=next.total-prev.total,busy=next.busy-prev.busy;
    const uint64_t expected=uint64_t(elapsed_ms)*tick_hz/1000;
    if(!expected || !total || busy>total ||
        uint64_t(total)*10<expected*8 || uint64_t(total)*10>expected*12)return -1;
    // Round up: 80.1% exceeds 80%, but exactly 80% does not.
    return int((uint64_t(busy)*100+total-1)/total);
}
