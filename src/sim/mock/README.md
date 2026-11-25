# Native mock boundary

Only the CMake simulator includes these headers and adapters. The production
PlatformIO target excludes `src/sim/` and continues to use the hardware libraries.

- `watch.hpp`: deterministic watch time, battery, brightness, display power,
  button edges, motion and haptic events.
- `storage.hpp`: standalone storage/NVS model, including capacity, removal,
  read-only state and exclusive USB ownership.
- `SD.h`: firmware-compatible volatile file handles backed by memory; no host
  filesystem access. `filesystem.cpp` exposes these to LVGL as drive A and seeds
  synthetic grid PNG map tiles. The SD facade does not emulate FAT or persist
  across process exit.
- `LilyGoLib.h`, `Arduino.h`: host SDK compatibility. Milliseconds follow LVGL
  ticks; microseconds advance in deterministic quanta inside busy-wait sampling.
- `esp_wifi.h`, BLE/GATT headers and `device_ui.cpp`: synthetic advertisements,
  in-memory radio state and reference-counted scan/stack users. No wireless I/O.
- `analysis_ui.cpp`, `security_ui.cpp`, `observation_ui.cpp`: finite UI fixtures
  for scans, detections, USB actions and passive/active-tool state. All results
  are simulated; no commands, keystrokes, captures or transmissions are issued.
- `nfc.hpp`: virtual NFC discovery and an NDEF record retained in memory, so the
  original write UI can write a value and the original read UI can read it back.
- `HTTPClient.h` and FreeRTOS task adapter: the finite voice-upload task executes
  synchronously against a fixed HTTP response, with no network transport.
  This models UI completion, not threading or real-time scheduling.

Original UI source, fonts and image assets are linked directly. Tests cover
screen entry/reentry, rendering, selected interactions and adapter state. These
mocks do not establish hardware correctness, radio timing or device performance.
