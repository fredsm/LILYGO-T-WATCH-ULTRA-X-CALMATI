#pragma once
#include <stdint.h>
#include <time.h>

// Value-only messages: no LVGL objects, borrowed buffers, or callbacks.
enum class UiEventKind : uint8_t { Clock, Battery, GPS, Timezone, Count };
struct UiEvent {
    UiEventKind kind = UiEventKind::Clock;
    uint32_t generation = 0;
    struct tm utc = {};
    int value = 0;
    bool first = false, second = false;
};
struct EventStats { uint32_t published=0, consumed=0, replaced=0; unsigned pending=0; };
void event_manager_init();
// Thread-safe task-context mailbox, one latest value per event kind. O(1),
// no allocation and never waits for a consumer. Not an ISR API.
bool event_publish(const UiEvent& event);
bool event_take(UiEventKind kind, UiEvent& event);
EventStats event_stats();
bool ui_task_is_owner();
// Arduino setup/loop already execute on Core 1. Retain that single owner.
void ui_task_claim();
