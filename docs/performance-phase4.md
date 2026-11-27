# Performance phase 4 — shared SPI arbitration

## 4A — bus audit
The Ultra SDK initializes global Arduino `SPI` with LoRa SCK/MISO/MOSI.
SD (including raw USB mass storage), RadioLib (SX1262), and the ST25R3916
NFC frontend use separate chip selects on that same bus. The OLED uses
the SDK's separate ESP-IDF QSPI host; neither that path nor I2C is changed.

## 4B/4C — transaction ownership
`spi_bus_manager` uses a static FreeRTOS mutex with priority inheritance.
Ownership begins before Arduino's native transaction lock and ends after
the native endTransaction unlocks. Only the global SPI object is routed
through the manager. The Arduino SD disk driver's native begin/end
references are linker-wrapped; the pinned RadioLib and ST25 drivers are
tagged by the idempotent PlatformIO patch script. The SDK explicitly passes
its already-initialized SPI instance to RadioLib, preventing the radio
from independently starting/stopping the bus. The wrapper uses the
Arduino 2.x SPI method symbols; re-audit when upgrading the framework.

No lock is taken around an entire filesystem operation, RFAL session, or
radio operation; doing so could invert the FatFs/SPI lock order. The
manager is task-context only, and calls must be paired and nonnested.
Existing driver chip-select and clock settings are preserved. IRQ
callbacks only publish pending work; they must not acquire this mutex.

## 4D — UI and lifecycle
Periodic radio and NFC UI jobs defer if the bus is active or has a waiting
task, before consuming packet flags. A deferral is advisory, not a
reservation; another transaction can start after the check. SD work
continues on the phase 3 worker. A transaction already in progress
cannot be preempted safely; synchronous interactive operations and
device busy/timeout handling can still stall the caller. No timeout
forcibly unlocks a live transaction.

TPMS OOK and the NFC background worker now stop cooperatively, so a
task cannot be deleted while holding SPI or continue touching a rail
after shutdown. Sleep waits for those producers and storage to finish
before the SDK powers rails down. Light sleep remains opt-in; full
peripheral resume coordination belongs to phase 9.

## 4E — diagnostics and tests
System Diagnostics shows current owner, active duration, waiters, and
per-client transaction counts, contention counts, UI deferrals, maximum
wait and maximum hold (microseconds, since boot). Counters are always
collected; the UI refreshes once per second while visible. Native tests
exercise cross-thread mutual exclusion, contention, and deferral and
verify vendor patch idempotence and failure on changed transaction anchors.
The desktop sim uses the real arbiter but does not simulate electrical SPI.

## 4F — physical watch acceptance (pending)
- Run SD writes and LoRa RX concurrently and swipe between screens; verify
  SD/LoRa counts grow, examine max wait/hold and UI stall history.
- Read an owned NFC tag during SD activity; check NFC count and card
  discovery. Check USB mass-storage read/write/eject/remount.
- Start/stop TPMS repeatedly during SD operations; confirm the bus returns
  to idle and other devices remain responsive.
- Exercise shutdown and long-press wake after active I/O; ensure no SD
  corruption or abandoned bus ownership.
- Compare phase 1 UI max-gap measurements with phase 4; this change alone
  cannot prove the origin of the 30–100-second display freezes.

Phase 5: I2C bus and sensor coordination. Phase 6: QSPI display DMA.
