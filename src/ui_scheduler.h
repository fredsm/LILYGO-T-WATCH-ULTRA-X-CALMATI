#pragma once
#include <stdint.h>

// Cooperative fallback for operations which still require the UI-owned buses
// or LVGL. Each pass selects at most one ready job and advances fairly,
// including when a job is temporarily ineligible (USB ownership/transition).
// A job is NOT preemptible: bus/storage managers must remove blocking calls.
class UiScheduler {
    unsigned cursor_ = 0;
public:
    template<class Ready>
    int next(unsigned count, Ready ready) {
        for (unsigned n = 0; n < count; ++n) {
            unsigned i = cursor_ % count;
            cursor_ = (i + 1) % count;
            if (ready(i)) return static_cast<int>(i);
        }
        return -1;
    }
};
