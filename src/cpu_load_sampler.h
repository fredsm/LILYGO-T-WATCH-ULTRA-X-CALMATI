#pragma once
#include <stdint.h>

// Shared per-core scheduler samples. Independent consumers keep their own
// baselines; diagnostics and the governor do not reset each other's counters.
struct CpuSamples { uint32_t total, busy; };
bool cpu_sampler_begin();
CpuSamples cpu_sampler_snapshot(unsigned core);
