# Performance phase 5 — shared I2C coordination

## 5A — audit

All listed devices use Arduino `Wire` (controller 0, SDA GPIO3/SCL GPIO2).
The SDK runs BHI260 firmware upload at 1 MHz and normal operation at 400 kHz.
GPS is UART, speaker/mic audio is I2S, radio/NFC/SD are SPI and the OLED is QSPI.

| Device | 7-bit I2C address | Work |
|---|---|---|
| AXP2101 PMU | 0x34 | button/power IRQ, rails, battery |
| Touch panel | 0x1A or 0x5A | IRQ-driven coordinates |
| DRV2605 haptic | 0x5A | vibration effects |
| PCF85063 RTC | 0x51 | clock and RTC interrupt |
| BHI260AP | 0x28 (alternate 0x29) | FIFO/motion IRQ |
| XL9555 GPIO expander | 0x20 | SD detect, reset, RF switch |

0x5A is deliberately labeled haptic/touch because address-only diagnostics
cannot disambiguate hardware variants.

## 5B — ownership and HAL integration

`i2c_bus_manager` owns a static FreeRTOS priority-inheriting mutex for
physical transfers on controller 0. Linker wraps the pinned Arduino
2.x `i2cWrite`, `i2cRead`, `i2cWriteReadNonStop` methods; repeated-start
register reads remain atomic. The native Wire lock stays in place.
There is no outer lock around Wire's shared buffers or blocking SDK
sessions, so the ordering is Wire -> manager -> native HAL.

Actual peripheral operations remain on the existing UI task, including
PMU rail changes for the alarm chime; the audio task only writes I2S
and publishes completion. Do not call Wire from workers or ISRs. The
native Wire receive buffer is not protected after requestFrom returns.
Future workers must use the UI/event owner or a complete device-operation
dispatcher, not just the HAL mutex.

## 5C — timeouts and recovery

Runtime transfer timeout capped at 50 ms (never increases a shorter
requested timeout); manager admission likewise waits at most 50 ms.
On a real HAL timeout, ESP-IDF 4.4's I2C master driver resets the state
machine and clears the bus before returning. The manager records the
timeout, skips transfers for 100 ms, and counts recovery on the next
successful actual transfer. This prevents repeated expensive calls within
one SDK operation. A NACK is tracked separately; it only defers optional
work targeting that client for 100 ms, not the entire bus. Boot scans
may report many expected NACKs.

No forced mutex release, manual GPIO clock pulsing, PMU reset, or automatic
power cycling is attempted; native driver recovery is used to avoid
corrupting an in-flight transaction or resetting shared peripheral state.
A permanently stuck electrical bus cannot be repaired in software.
The bounds cover HAL transfer/admission, not every high-level library
call or a task stuck holding the original Wire lock.

Boot (including BHI firmware and the deep-wake gate) and vendor sleep
transitions retain upstream timeouts without cooldown or suppression.
Normal 400 kHz is restored after light sleep.

## 5D — interrupt and status policy

SDK GPIO ISRs remain event-bit-only. PMU IRQ is serviced before sensor
work; failed reads retain pending events and do not clear unread PMU
status. If clearing PMU IRQ fails, retry it; partially cleared IRQs can
still cause duplicate events or loss at the peripheral. Touch I2C faults
cancel the current LVGL gesture and discard it until a real release
rather than producing spurious clicks. Sensor FIFO errors retain the
pending event. Nonessential vibration is skipped during backoff.

RTC cache advances every second and resynchronizes every 30 seconds,
plus immediate reads following clock adjustment. Battery polling is
every five seconds. SD detect and NFC rail status remain at one second.
A failed status read retains the last valid value; an expander failure
never falsely mounts/removes SD. Motion retains its existing 10 Hz
sensor-driven update and no new sensor polling loop is introduced.

## 5E — diagnostics and test

System Diagnostics: per-device transactions, NACKs, timeouts, other
errors, deferrals, max wait/transfer duration; bus active owner, admission
timeouts, cooldown and recovery count. Always collected; startup probes
are included. Native tests exercise mapping, mutual exclusion, bounded
admission, timeout vs NACK, backoff, recovery, lifecycle bypass and
concurrent transfers. The headless and desktop simulator use the actual
manager for these tests; they do not simulate electrical devices.

## 5F — watch acceptance (pending)

- Use touch, motion wake, power button, alarm/timer audio, and SD
  insertion/removal during WiFi/LoRa activity. Check diagnostics and UI
  stall history; verify correct button hold behavior.
- Verify RTC/manual/GPS/NTP time changes and battery/charging updates.
- Confirm no false card detach, stale press, or lost pending IRQ during
  transient I2C errors; observe recovery count.
- Test light/deep sleep and wake; verify touch, PMU and motion after wake.
- Capture serial logs and compare maximum I2C transfer and UI gap times
  with phase 4.

Phase 6: dedicated display QSPI and DMA.
