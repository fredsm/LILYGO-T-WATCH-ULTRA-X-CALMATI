# Phase 10 — CPU governor tuning and performance verification

## 10A — Demand estimation

The original CPU preference remains 240 MHz; Auto is opt-in from
Settings > CPU Frequency. Fixed 80/160/240 MHz modes are unchanged.
There is one shared pair of FreeRTOS tick hooks for diagnostics and
Auto, not an additional polling loop. The existing timer notifies a
blocked governor worker approximately every 500 ms. Coalesced/missed
timer events are not replayed as synthetic load windows.

Both diagnostics and the governor use the same wrap-safe tick-window
validation: 400–1000 ms duration, 80–120% expected tick coverage, busy
count no greater than total, and nonzero total. Busy percentages round
up, preserving exact threshold behavior. Tick sampling measures
approximate scheduler occupancy, not instruction counts or interrupt
utilization. Each consumer retains independent baselines.

## 10B — Three tiers, with downshift protection

The maximum of the two core loads determines the raw target:
- Either core >80%: 240 MHz.
- Else either core >50%: 160 MHz.
- Otherwise: 80 MHz.

Upshifts take effect on the next valid window, including direct 80 to
240. Downshifts require five seconds of consecutive windows with the
same safe lower target. A different target or invalid sample clears
the dwell. A lower frequency increases occupancy for an identical
CPU-bound workload, so the governor additionally estimates its demand
at each lower clock and admits 80 MHz only if the busiest core's
projected load is <=45%, and 160 MHz only if <=75%. Five percentage
points of headroom reduce chatter near the existing upshift thresholds.
This is a conservative estimate, not a measurement at the new rate.

For example, a constant 48 MHz-equivalent workload would report
20% at 240 MHz, 30% at 160 MHz, and 60% at 80 MHz. Unprotected
downshifts would repeatedly select 80 then 160 MHz; the new governor
settles at 160. Very light demand can still go directly 240 to 80.
The safe target can therefore exceed the raw target. This intentional
stability change affects Auto only.

## 10C — Invalid samples, sleep and failures

An invalid window never changes the current frequency, regardless of
how many consecutive windows are bad. It clears any pending downshift.
Once good windows return, the usual immediate upshift policy applies
and normal five-second downshift qualification restarts.
A rejected clock transition is counted; automatic requests are
reevaluated on the next window. No tight retry loop. Manual choices
are not overridden by missing CPU samples.

The existing light/deep sleep lifecycle pauses the governor and clears
its dwell/sample baselines. Auto resumes at 240 MHz with fresh
samples; a fixed frequency is unaffected. Screen-off alone retains
governor operation for background work.

## 10D — Diagnostics

Tools > Task Mgr > Diagnostics CPU shows the current frequency/mode,
per-core utilization, raw and safe targets, downshift dwell, valid and
invalid windows, guarded windows, successful up/down transitions,
consecutive invalid samples, rejected clock changes and maximum automatic
clock-change duration. Sampled seconds by frequency count **valid Auto
windows only**, not all wall time, interrupts or deep sleep. Cumulative
counters survive an Auto pause/manual selection but reset at boot.
The existing independent UI heartbeat, stalls, task slices, resource
ownership and bus/worker diagnostics remain available.

## 10E — Regression and physical acceptance

Host checks exercise both-core thresholds, projected load stability,
five-second dwell, boundary rounding, counter wrap, bad tick coverage,
stale windows, hold-on-invalid recovery, rejected clock changes, preference
persistence, screen-off and sleep/resume behavior. Mock load injection
represents per-core utilization at the current clock, not actual host
CPU usage. The simulator does not model electrical/thermal or radio
timing impacts of clock transitions.

Physical watch acceptance:
1. Use fixed 240 MHz as the baseline; measure UI stall count/max gap,
   QSPI errors/timeouts, SD queue/latency, I2C failures and SPI wait.
2. Repeat the same routine in fixed 160 and 80 MHz and Auto: clock
   scrolling, settings, SD image access, GPS, audio, and normal WiFi
   connectivity. Use a consistent SD card, brightness and radio state.
3. Confirm Auto starts at 240, eventually falls on light demand, steps
   directly to a higher tier on a high-load sample, and does not
   repeatedly alternate tiers for a steady workload. Record invalid
   and guarded windows and clock errors.
4. Repeat screen-off/power-button wake, intentional light sleep (motion
   wake disabled and services idle), and long-hold deep sleep/wake with
   each preference. Check persisted settings and no stale sample dwell.
5. Compare the diagnostic UI max-gap/stall results and measured current
   using the same workload and duration. Lower clocks are only useful
   if responsiveness and peripheral operation remain satisfactory.

Phases 1–10 address software architecture and measurement. Successful
builds/host tests do not prove that the original 30–100 second physical
display freezes are gone. Real-watch stress/current measurements are
required before choosing Auto as the default or claiming a power
savings percentage.
