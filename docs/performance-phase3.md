# Architecture V2 — Phase 3: asynchronous SD storage

## 3A — Storage manager

Core 0 runs a low-priority, notification-driven `sd_worker`. The FIFO uses a
fixed 12-entry descriptor ring and a 2 MiB in-flight payload budget (large
allocations use PSRAM). Submission never waits for the worker or allocates an
unbounded backlog. An accepted request owns its immutable bytes, including its
copied path; an owned submission consumes the buffer even on rejection. The
worker does not touch LVGL, PMU, RTC, GPS or other UI state.

Two queue entries and 8 KiB are reserved for settings replacements. Pending
replacements of the same settings path coalesce to the latest value. Other
requests preserve FIFO ordering. Write failure, queue rejection, cancellation
and coalescing are separate counters. Acceptance means queued, not durable.
Filesystem writes retain the prior on-card formats; there is no crash-atomic
rename or power-loss guarantee.

## 3B — Write migration

- Wardriver: only the compact valid-record snapshot is copied on Core 1; CSV
  formatting and the full-file rewrite run on the worker. A full session
  isn't cleared for rollover when submission fails. Final snapshots are
  retried when the queue has room. Removal drops unsent data for that medium.
- Screenshots: LVGL captures on its owner; the BMP bytes are copied into an
  independent PSRAM buffer before the draw buffer is destroyed. The worker
  writes the file in bounded 4 KiB chunks.
- Settings, alarm, timezone, routine detector logs, APRS/pager and Meshtastic
  message/node archives: existing formatted records are built in a bounded
  RAM buffer and handed to the worker. Directory creation is part of the
  worker operation, rather than an extra filesystem call on the UI.
- Existing direct operations are protected by media-lifecycle leases.
  Audio streaming, arbitrary map and image reads, web-file streaming,
  remaining specialized log formats and configuration reads/writes still
  have synchronous callers. The queue is not a replacement for the shared
  SPI manager (Phase 4) or audio lifecycle (Phase 8).

The firmware's Arduino SD disk driver takes a SPI transaction lease via
`AcquireSPI`; ESP-IDF FatFs has reentrancy enabled. Legacy readers and the
worker rely on those native per-operation locks, not a new global mutex that
could block the UI for an entire snapshot. The app-level lease protects
mount/ownership lifetime, not arbitrary simultaneous access to the same file.
Phase 4 must verify and coordinate every shared bus client's transaction
boundaries. A slow card can still delay a synchronous legacy or radio call.

## 3C — Read migration

The USB-SD usage screen queues the expensive FAT capacity/used-bytes query
and renders the cached result, rather than walking the FAT on the UI task.
POSIX LVGL file/directory open/close callbacks are wrapped in lifecycle
leases, including map-image handles.

## 3D — Lifecycle and data integrity

- USB MSC mounting is refused while a worker job, queued work or local open
  file is present. No forced cancellation is needed. A busy message is shown;
  the user can retry. The raw USB callbacks have separate leases. On eject,
  new raw I/O is blocked and local access stays closed until the last callback
  finishes and the local FAT filesystem has been remounted.
- On card removal, new work is rejected and pending jobs are cancelled.
  The driver is not uninstalled until active jobs, file handles and raw USB
  callbacks have released their leases. The worker never remounts the card.
- Deep sleep stops producers, finalizes the recorder, retries the final
  wardriver snapshot when possible, then stops accepting work and drains
  accepted requests before the SDK shuts down SD/SPI. With a stalled SD
  operation, shutdown may consequently take longer than the key hold;
  forced teardown of a live writer is deliberately avoided. A hardware
  power cut while a write is in progress remains unsafe.
- Light sleep is deferred if the storage lease is not idle.

## 3E — Regression

Native tests use the production queue with a deterministic deferred backend.
They cover payload copying, FIFO, saturation, preference coalescing/reserve,
write errors, removal cancellation, read metadata, live LVGL file leases,
USB handoff/remount and draining. Existing UI tests cover navigation,
settings, storage fixtures, power, PIN and shutdown. The desktop mock
normally completes requests inline and does not invent real hardware latency.

## 3F — Hardware profiling

Diagnostics shows queue depth/payload bytes, active work, completed, failed,
rejected, cancelled and merged jobs, and maximum worker job duration.
Compare UI gap and stall history versus Phase 2 with the same fixed 240 MHz
workload, particularly periodic snapshots, screenshots and routine logging.
Test full queue, card removal/reinsert, PC mount/eject during writes, and
deep-sleep shutdown with queued requests. Confirm output files and settings
after reboot. Physical validation remains pending.
