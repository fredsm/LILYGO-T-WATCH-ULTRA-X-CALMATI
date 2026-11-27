#pragma once
#include <stdint.h>
// UI owns power and UART begin/end; stop the session BEFORE driver teardown.
// Worker reads/parses the active UART and publishes immutable fixes.
void gps_service_init();
void gps_service_session(bool on);
void gps_service_tick();
uint32_t gps_service_backlog();

struct GpsServiceStats {uint32_t bytes=0, sentences=0, backlog_peak=0, saturated_reads=0;bool worker=false;};
GpsServiceStats gps_service_stats();
