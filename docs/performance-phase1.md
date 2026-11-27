# Architecture V2 — Phase 1: System Diagnostics

This phase is observational. It does not change tool ownership, hardware bus
arbitration, LVGL rendering mode, or the CPU governor's three-tier policy.

## Running the baseline

1. Use Settings > CPU Frequency > 240 MHz for initial testing.
2. Connect USB serial at 115200 baud if available; keep the host reading.
3. Reproduce the freeze (note tool, SD installed, and active radios).
4. Open Tools > Task Mgr > Diagnostics after recovery. Record the recent stall,
   slowest operations, and memory headroom. A typical report is
   `[diag] UI recovered 30000ms | Wardriver / SD`.
5. Repeat with the SD card removed (power off first), or with each radio enabled
   individually, to narrow the suspected dependency.

## How measurement works

- Main-loop operations are wrapped with scoped, monotonic timestamps. LVGL
  refresh events separately mark synchronous display flush and wait time.
  A UI service gap is the interval between completed LVGL handler calls, not
  proof that the physical panel produced a new frame.
- A core-0 monitor checks the heartbeat about every 50 ms. It captures active
  stalls at >=100 ms and the UI completion path records shorter-lived stalls
  that recover between monitor samples. The likely culprit is the longest
  observed operation during the gap; nested operations can overlap.
- Latest 12 events are held in a fixed RAM ring-like history (newest first).
  The UI displays the five most recent. A reboot clears the history.
- A separate, lower-priority telemetry task samples memory once per second and
  attempts USB logging. The heartbeat monitor does not use USB, SD, SPI, I2C,
  dynamic allocation, or the LVGL API.
- Both diagnostics and the Auto CPU governor use one shared pair of tick
  hooks. Each independently calculates approximate per-core active scheduler
  occupancy from the latest 500 ms sample. No frequency adjustment in a fixed
  mode. Invalid/missed tick windows are reported as unavailable.
- Intentional light/deep sleep and the qualified shutdown hold suppress stall
  warnings. Screen-off alone does not stop main-loop monitoring.
- Operation durations include driver calls and waits; this is not an
  electrical bus trace, and worker internals are not instrumented yet.
  If both cores or interrupts stop executing, the independent monitor cannot
  report live; a recovered UI gap may still be recorded, or a reboot may lose
  the RAM history. The existing boot banner reports the reset reason.

The two diagnostic tasks request 3 KB stack each, in addition to small fixed
state and LVGL objects created on demand. A task allocation failure does not
prevent normal watch startup. Memory sample age exposes stale telemetry.

## Simulator

Open the same Diagnostics screen. The F6 panel supplies simulated core demand
and a **Simulate 30s SD stall** button. This injects a recovered stall without
delaying the desktop or advancing the watch clock. Memory values are fixtures;
the simulator does not measure physical bus or scheduler performance.

Phase 2 (UI separation), asynchronous storage, and bus ownership remain future
work and should be informed by physical-watch measurements.
