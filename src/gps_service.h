#pragma once
#include <stdint.h>
// UI owns power and UART begin/end; stop the session BEFORE driver teardown.
// Worker reads/parses the active UART and publishes immutable fixes.
void gps_service_init();
void gps_service_session(bool on);
void gps_service_tick();
uint32_t gps_service_backlog();
