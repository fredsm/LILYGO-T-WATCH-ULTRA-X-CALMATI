# Architecture V2 — Phase 2: UI ownership and event scheduling

Phase 2 establishes a single LVGL owner, a cross-task event layer, background
workers for independent services, and an input-first cooperative dispatcher
for the remaining hardware/UI-coupled services. Arduino's existing Core 1
setup/loop task is the sole UI owner. Its priority remains the SDK default:
raising the priority of a synchronous bus operation would not make it safe.

## 2A — Event manager

- Fixed, typed, value-copy latest-value mailboxes for clock, battery, GPS,
  timezone and weather. Short critical sections; no heap, blocking queue waits,
  LVGL objects or borrowed buffers. Independent sources cannot overwrite each
  other. Data events intentionally coalesce.
- Back GPIO interrupt uses an independent IRAM-safe latched edge, so telemetry
  cannot replace a button event. Mechanical bounce is merged; the existing
  250 ms debounce is retained. PMU power edges and touch remain dispatched on
  the UI task through the existing drivers, preserving hold and PIN semantics.
- Simulator checks the actual owner thread instead of always returning true.

## 2B — UI task and cooperative dispatcher

- PMU/power, LVGL, Back/PIN/touch, GPS result application and inactivity are
  serviced before lower-priority work.
- Legacy background calls are round-robin jobs, with a 4 ms cooperative
  budget, checked **between** jobs. At most one execution per job per slice;
  subsequent slices resume fairly at the next job. Rendering and input
  processing resume between slices; screen transitions defer optional work.
  USB-SD ownership gates and PIN screenshot protection are retained.
- The 1 Hz clock/battery/alarm status pass is no longer coupled to the
  wardriver disk flush. Serial console input is limited to 128 bytes and one
  complete command per invocation.
- This is not preemption of a synchronous driver. LVGL callbacks, PMU/touch,
  radio lifecycle, SD accesses and shared bus transactions can still block
  the UI task. Full independent UI execution requires the managers in
  Phases 3–7, not a second competing task calling LVGL.

## 2C — Background services

- GPS UART ingestion and NMEA parsing both run on Core 0. The worker sleeps
  indefinitely while GPS is disabled and waits 10 ms between bounded reads
  while active. UI retains GPS rail and UART begin/end ownership; session
  stop synchronizes only with the bounded UART read before driver teardown.
  Copied TinyGPSPlus fields are applied only on the UI owner. Generation
  checks reject stale fixes after GPS power cycles and sleep. A bounded UI
  fallback remains if task allocation fails.
- Wi-Fi NTP/geolocation worker runs on Core 0 and blocks on notification.
  It publishes a copied result; RTC writes and timezone persistence remain
  on the UI owner (manual time still takes precedence).
- Weather fetch stays on Core 0, with immutable request data and
  copied progress/completion events. UI alone applies sample/status changes;
  credential fallback (which may access SD) is on the existing owner.
  NVS sample persistence uses the worker's own local result. Existing Wi-Fi
  ownership behavior is unchanged until Phase 7.
- Drivers and services sharing SD, SPI, I2C, RF lifecycle or LVGL are NOT
  arbitrarily started on competing cores. Instead their existing ownership
  is preserved via the cooperative dispatcher until their dedicated phases.

## 2D — Cached UI data

RTC/battery are sampled once per second on the existing I2C owner, then the
UI uses cached values (clock extrapolates between samples). Manual/GPS/NTP
clock writes refresh immediately. Task Manager and Meshtastic reuse cached
status. GPS and weather updates are copied; no worker writes live UI state.

## 2E — Regression tests

Native tests exercise concurrent mailbox publication, coalescing,
owner identity, independent Back latch, value-copy weather results and
round-robin scheduling with temporarily ineligible jobs. Existing mock UI
tests cover clock, navigation, buttons, screen-off, PIN, power-off and wake.
The mock does not simulate the real UART, bus latency or RTOS scheduling.

## 2F — Measurement and physical acceptance

Diagnostics now reports data event pressure, merged Back edges, peak observed
GPS UART backlog (not a count of dropped bytes), maximum cooperative slice
duration and count of slices over 4 ms, in addition to existing UI gap, stall
history and memory/CPU telemetry. Slice statistics cover cooperative jobs only;
LVGL, status and power operations retain separate diagnostic attribution.

On the watch, compare with Phase 1 at fixed 240 MHz:

1. Cold boot; record firmware commit, max UI gap, stall count, minimum
   heap and largest free block. Repeat the same workload after flashing
   Phase 2 (idle, frequent screen transitions, GPS and normal logging).
2. Check navigation and power-key response during background activity.
   Record the culprit and duration of every gap >=100 ms.
3. Enable/disable GPS repeatedly, verify fresh fixes, then test screen-off
   wake and a 5-second shutdown/5-second wake. Check UART backlog and ensure
   old fixes are not displayed across power cycles.
4. Compare UI gaps and operation timings, especially SD, Wi-Fi/BLE, NFC and
   I2C. Long cooperative slice times identify jobs to migrate first in the
   subsequent storage/bus phases. Re-run at Auto after fixed-240 comparison.

Stage 2F instrumentation is implemented; actual hardware measurements and
acceptance remain pending. This change does not guarantee removal of
30–100 s freezes caused by a single blocking operation.
