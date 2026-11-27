#pragma once
#include <stdint.h>
// UART handoff: only the UI owns Serial1 power/read operations. A CPU-only
// worker parses copied data; the UI applies the latest immutable result.
void gps_service_init();
void gps_service_session(bool on);
void gps_service_tick();
uint32_t gps_service_dropped();
