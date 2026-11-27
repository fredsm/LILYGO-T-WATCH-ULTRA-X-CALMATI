# Architecture V2 — Phase 2: staged UI/event foundation

This is the first migration of Phase 2. It keeps Arduino's existing Core 1
setup/loop task as the single LVGL owner, not a second competing UI task.
An LVGL heartbeat is serviced before the old background calls as well as at
the existing render points. The SDK's task priority is unchanged: giving the
entire legacy loop higher priority would also elevate its blocking I/O.

## Stage 2A — Event manager

`event_manager` supplies fixed, typed, value-copy latest-value mailboxes for
clock, battery, GPS and timezone. Publication and consumption are protected by
a short critical section; no heap, blocking queue waits, LVGL pointers or
callbacks are passed between cores. Pending/replaced counters are exposed in
Diagnostics. The mailboxes deliberately coalesce data snapshots, not ordered
input or command events. The existing PMU and Back-button path is retained
until its driver ownership can be migrated safely.

## Stages 2B–2D — First background and data migrations

- GPS: the UI still manages UART power and takes at most 128 available
  bytes per iteration; a Core 0 worker blocks on a fixed six-chunk queue
  and parses NMEA, publishing immutable copied fields. Existing screens
  and tools still consume the same UI-owned TinyGPSPlus fields. GPS power
  cycles advance a generation so delayed results cannot cross sessions;
  sequence discontinuities reset the worker parser after overflow. Queue
  overflow counts dropped bytes, without waiting for the worker.
  If the worker cannot be allocated, bounded parsing remains on the UI owner.
- Wi-Fi NTP and geolocation: the existing worker is pinned to Core 0 and
  waits for an explicit notification rather than polling a volatile flag.
  It publishes a copied result; RTC writes, timezone persistence and LVGL
  updates remain on the UI owner. Manual time still takes precedence.
- RTC and battery: sample once per second on the existing I2C owner and
  render from cached values; the RTC cache extrapolates between samples.
  Manual/GPS/NTP clock writes refresh immediately. No new concurrent I2C
  access is introduced. Task Manager and Meshtastic reuse cached status.
- Sleep: GPS parser sessions stop accepting old results before shutdown;
  light wake starts a fresh session if GPS is enabled. The worker never
  accesses UART/PMU or LVGL.
- Diagnostics: pending/coalesced event counts and GPS dropped bytes.

## Stage 2E — Regression tests

Native tests exercise the shared event manager under a concurrent producer
and consumer, and existing mock UI tests cover clock, navigation, buttons,
screen-off, PIN, power-off and wake. Simulator GPS and device fixtures are
retained; the hardware parser queue and RTOS scheduling require real hardware
validation.

## Stage 2F — Measurements and remaining work

Compare max UI gap, recent stalls, queue pressure, GPS drop counts and heap
with Phase 1 at fixed 240 MHz. The change is not a guarantee of eliminating
30–100 s freezes: the legacy Wi-Fi/BLE switching, SD logging, shared radio/NFC
operations, PMU/touch and network server still have synchronous UI paths.
Moving those into arbitrary workers before the relevant bus, storage and
radio lifecycle managers exist could create races or corrupt files. Those
paths are intentionally retained pending Phases 3–7; complete isolation,
actual timing and hardware responsiveness remain to be verified.
