# Phase 9 — Peripheral power and sleep/resume lifecycle

## 9A — Power states and ownership

The UI owns the power lifecycle. Screen off, light sleep and deep sleep
are different states. Screen off pauses refresh, finishes outstanding QSPI
DMA, sleeps the panel, and gates the display rail. Wake reverses that
sequence and only enables LVGL input when panel/brightness restoration
has completed. I2C errors or quarantined DMA defer the transition.

The user-selected 60-second default ScreenOff Timer (Off / 60 / 3000 s),
PIN lock and five-second power-button hold/release contract are unchanged.
The PIN lock is independent of the temporary input lock used for sleep.

## 9B — Retentive light sleep

Five seconds after the screen-off transition, the power manager may use
ESP light sleep. It does not use LilyGo's destructive lightSleep helper,
which unmounts SD and unconditionally enables peripheral rails on wake.
The existing SPI, UART, I2C, SD, application state and radio preferences
are retained. No peripheral is restarted merely because the CPU wakes.

Idle eligibility is intentionally conservative: active radio controllers
or jobs, audio, GPS, LoRa, NFC, onboarding, pending storage, USB/VBUS,
alarm/snooze/timer/stopwatch, motion wake, QSPI, I2C, SPI or a pending PMU
IRQ veto sleep. When blocked, normal screen-off behavior continues;
eligibility is rechecked every five seconds. The native motion-wake
setting defaults to enabled, so disable it to exercise automatic light
sleep; saved WiFi auto-connect may also prevent entry. ScreenOff Timer
Off disables automatic screen-off/light-sleep entry.

Before sleep, audio/radio/storage admission is closed and a nonblocking
SPI lifecycle reservation is acquired. Diagnostics and the CPU sampling
governor pause. The ESP wake source is the PMU interrupt, including its
latched level; a key press wakes and turns on the screen, whereas other
PMU interrupts can wake the CPU without turning on the screen. A failed
sleep attempt restores ownership/admission as on wake. The normal CPU
preference remains unchanged; Auto restarts with a fresh sample window.

## 9C — Staged deep sleep

The current power-off gesture is a five-second hold followed by release
(to avoid immediately waking on a still-held power key). It immediately
locks input and turns off the screen, then advances the following stages:

1. **Stopping workers:** stop admitting new managed audio/radio work,
   request GPS, audio/voice, ADS-B, TPMS and NFC worker shutdown, and stop
   USB media exposure. Wait for worker/transaction acknowledgment.
2. **Final storage:** allow the existing final storage submission, wait
   for accepted work, release cached image leases, and close admission.
3. **Draining:** wait for storage leases, QSPI completion, I2C and SPI.
4. **Drivers:** reserve radio/SPI lifecycle access; verify WiFi and BLE
   controllers are actually stopped and recheck ownership. Arm PMU
   power-key-only wake and hand off to the vendor full peripheral shutdown.
5. **Deep sleep:** vendor shuts down the display, SD, remaining peripheral
   rails and buses. Only the power-key IRQ is enabled. A short wake press
   is rejected by the existing early boot gate; a five-second hold boots
   normally and restores saved settings/PIN state.

Stages advance on successive UI ticks, not an unbounded spin waiting
for a worker. There is deliberately no force-after-timeout fallback
that tears down a driver beneath a live owner. A stuck native driver,
uncooperative worker or unrecoverable bus failure can delay shutdown;
the screen remains off. Some vendor/SDK calls remain synchronous
(including USB stop and final deep sleep rail settling). This is not yet
a guarantee of bounded native shutdown latency or measured current.

## 9D — Boot and diagnostics

Boot turns off the probed GPS UART/rail and speaker amplifier to match
the initial UI state; later owners enable them when needed. Existing
NFC-off boot behavior is retained. System Diagnostics reports current
power stage, idle veto mask, light sleep entry/wake/abort counts, last
sleep duration, and last shutdown drain duration and observed veto mask
(retained across intentional deep-sleep reboot). Duration measures
software drain, not the vendor's final rail-settling time.

## 9E — Verification

Desktop simulator uses the same state machine and admission gates,
mock power button, screen state and deep wake gate. Native-only PMU,
bus-health, motion and physical-driver details are not modeled.
Regression tests cover deadline veto, deferred shutdown, storage
admission restoration, exclusive SPI reservation, stage order and
millisecond counter wraparound. The LilyGo patch must be idempotent.

Watch acceptance (pending):
- Confirm screen off at 60 seconds, brief power-key wake, and retained
  PIN lock. Test screen timer Off and 3000 seconds.
- Disable motion wake and disconnect/disable radios and active services;
  confirm light sleep after the five-second grace and short key wake,
  repeated cycles, brightness, touch and SD/GPS state.
- Verify screen-off-only operation while GPS, radio, chime, timer,
  alarm/snooze, USB, or motion wake is active.
- Power off during storage and audio work; confirm drained data and safe
  device shutdown. Verify touch, Back and charging cannot wake deep sleep,
  short power-key wake is rejected, and five-second hold boots normally.
- Review Diagnostics and measure actual screen-off, light-sleep and
  deep-sleep current, wake latency, and native shutdown timing.

Phase 10: CPU governor tuning and final performance verification.
