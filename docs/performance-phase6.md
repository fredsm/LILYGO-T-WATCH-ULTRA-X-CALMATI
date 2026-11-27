# Performance phase 6 — dedicated QSPI display pipeline

## 6A — audit

The Ultra OLED (CO5300, 410 x 502 portrait) uses SPI3_HOST, separate
from SD/LoRa/NFC. The LilyGo SDK configures QSPI at 80 MHz, manual CS,
and leaves its synchronous DMA option and tearing synchronization off.
The LVGL helper allocates two full-screen PSRAM buffers (~804 KiB in
total); its flush callback swaps RGB565 and calls synchronous
`pushColors`. Even the stock DMA option queues a chunk and immediately
waits for it. PSRAM transfers can require temporary internal DMA buffers.
The earlier 8 KiB chunk patch reduced allocation failures under WiFi/BLE
load but did not remove runtime allocation and blocking waits.

## 6B/6C — bounded asynchronous DMA

The stock initialization and boot clear remain intact. After
`beginLvglHelper`, firmware owns the runtime flush callback and wait
callback. Board offsets, CASET/RASET/RAMWR commands, manual CS and quad
pixel protocol are unchanged. QSPI clock remains 80 MHz; TE busy-wait
is not enabled.

- Two 8 KiB partial LVGL buffers live in PSRAM (single-buffer fallback if
  the second allocation fails), preserving the vendor's area rounder.
- One 8 KiB statically allocated, 4-byte-aligned internal DMA staging
  buffer and persistent transaction descriptor. RGB565 swap happens in
  the staging copy, not the LVGL buffer. No runtime pixel DMA bounce
  allocation is required for the aligned transfers.
- Send the three panel window commands with bounded queue/result waits.
  Queue the pixel transfer asynchronously; LVGL can render the next
  partial region while the previous transfer runs. The final result is
  reaped by the LVGL wait callback or a nonblocking main-loop service.
- Display command writes (brightness, rotation, sleep/wake) and legacy
  synchronous pushes use the same runtime engine; no mixed polling/queued
  transfer and no overlapping CS.
- No additional rendering task or LVGL calls from ISR/worker context.

The small partial buffers save PSRAM; the 8 KiB internal staging buffer
is a deliberate fixed SRAM cost to eliminate dynamic DMA allocations.
More windows/commands per full frame are a tradeoff; raw rendering speed
and visual quality require on-device measurements. Default LVGL refresh
period remains 33 ms; no screen appearance changes.

## 6D — faults and power lifecycle

One bounded wait (20 ms) per accepted transaction; queue admission is
nonblocking. An unconfirmed transfer is quarantined: CS stays asserted,
the descriptor and staging memory are not changed, and subsequent
calls only poll. LVGL is free to release its own render buffer because
DMA never references it. No new window/command is issued until the
outstanding result is returned. Failed flushes are skipped; late
completion or queue rejection triggers a 100 ms cooldown then full
screen invalidation on the UI owner. An error on a window command
prevents pixels being sent to the previous window.

Pause refresh before screen-off; wait for confirmed SLPIN before gating
the display rail. Wake restores rendering only after panel commands
complete. Light sleep aborts safely if the panel cannot be quiesced;
deep sleep defers while an outstanding transfer cannot be reaped.
No forcibly freed DMA memory or controller reset while DMA is active.
A permanently unresponsive DMA engine may require hardware emergency
power-off/reset; software does not silently abandon its ownership.
Startup SDK boot transfers are still synchronous.

## 6E — diagnostics and tests

System Diagnostics shows asynchronous DMA state, flushes, completions,
dropped flushes, command count, timeouts/errors, late recoveries, maximum
submission/wait/queued-to-reap duration, bytes and staging size.
Queued-to-reap is wall-clock latency including scheduling, not pure
wire time. Native test uses the production pipeline state machine with
a fake transport to exercise correct window/pixel encoding, asynchronous
ownership, timeout, quarantine, nonblocking poll, late recovery, redraw,
queue rejection, command failure and paused state.

Set `TWATCH_QSPI_ASYNC=0` in platformio.ini for an A/B build using the
original synchronous driver/buffer path (no new Settings UI). The new
path is enabled by default. Physical mock does not emulate QSPI timing.

## 6F — physical watch acceptance (pending)

- Flash and inspect full-screen and partial redraws: clock, swipe,
  scrolling, keyboard, image/snapshot, rapid brightness changes.
- Stress WiFi + BLE scan alongside SD writes and UI navigation. Record
  min internal heap, display max wait/submit, dropped frames and UI stalls.
- Screen-off, power-button wake, PIN lock, long-press deep sleep;
  verify refresh resumes and no pending DMA at power down.
- Compare asynchronous and original builds if tearing/color corruption
  or throughput regression occurs. Keep the original panel timing until
  measurements justify a change.

Phase 7: network/radio coordination.
