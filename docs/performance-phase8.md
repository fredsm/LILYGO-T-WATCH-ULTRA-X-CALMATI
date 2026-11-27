# Phase 8 — GPS and audio subsystem scheduling

## 8A — audit

Phase 2 already moved NMEA parsing to Core 0. The remaining cross-core
exception was the aircraft-data worker reading and mutating TinyGPS++
fields concurrently with the UI. The microphone reader was on Core 0,
but an LVGL timer loop still drained audio to SD, including potentially
slow writes and WAV finalization. WakeNet model loading/stop and voice
upload preparation also performed synchronous work on the calling task.
The legacy LilyGo PDM readBytes and Player write wrappers wait
indefinitely (Player ignores its supplied timeout).

## 8B — GPS

- UART RX ring increases to 1024 bytes at regular power-on, using the
  pinned Arduino HardwareSerial API before begin().
- Worker reads up to 256 available bytes in one zero-timeout bulk read
  every 10 ms. On allocation failure the same bounded UI fallback remains.
- Session generation protects a scalar latitude/longitude/fix-time snapshot;
  disabling GPS invalidates it and late prior-generation publishes fail.
- Aircraft worker consumes that snapshot, preserving its last-known-center
  behavior. UI still owns the compatibility TinyGPS++ fields, RTC updates
  and GPS power control. Other UI consumers retain their existing behavior.

## 8C — audio

- Separate I2S microphone and speaker admission tokens. Recorder and
  WakeNet cannot read the same microphone simultaneously. Chime may use
  the independent speaker port. Stale release tokens cannot release a
  new session.
- Capture uses the vendor PDM read API with a 100 ms wait instead of
  readBytes(portMAX_DELAY); repeated empty reads stop the session.
- Recording producer on Core 0, priority 3; writer on Core 0, priority 1.
  A 64 KiB PSRAM stream buffer provides ~2 seconds of slack. The writer
  owns all file creation, writes, header patch and close. Storage lifecycle
  lease holds until the file is closed; no LVGL timer writes SD.
  A 40 ms buffer admission wait plus short-write detection stop recording
  on overrun rather than silently dropping continuous samples. A partial
  file is marked as such; only successfully finalized WAV is uploadable.
  Existing 16 kHz mono 16-bit WAV and ten-minute hard cap retained.
  If all 1000 numbered slots exist, fail rather than overwrite.
- WakeNet model load/detect/destroy is asynchronous, Core 0 priority 1;
  no forced shutdown of someone else's WiFi/BLE session. Stop requests
  do not join on the UI; token releases after actual cleanup.
- Chime directly uses IDF i2s_write with a 100 ms wait, reports short
  writes, and leaves amplifier rail management on the UI. The SDK clock
  setup is still a native worker call.

## 8D — voice pipeline and power

Voice recorder owns the Phase 7 radio lease while it needs WiFi off;
standalone recording retains its original radio behavior. After the
correct recording generation finalizes, a worker acquires the voice
radio lease, reads WAV and WiFi configuration on SD, connects, and does
HTTP. The UI receives text only after the completion flag is published.
On UI timeout, request cancellation; do not allow overlapping upload
buffers. Existing assistant endpoint and intentional WiFi connection
behavior are retained. The pinned HTTPClient timeout argument is
uint16_t; set 60,000 ms rather than overflowing 90,000. Cancellation
between native network operations is cooperative, not a hard abort of
a POST already in progress.

Light sleep refuses a live audio owner. Deep sleep closes audio and
managed radio admission, requests recording / wake-word / chime stop,
and retries until ownership is released; an active HTTP request or
vendor/SD operation may still delay final shutdown. Complete peripheral
rail restoration and driver recovery are Phase 9.

## 8E — diagnostics and tests

System Diagnostics: GPS bytes, valid sentences, full reads/backlog peak,
worker/fallback; audio mic/speaker owners, rejected and completed
sessions, I2S read/write errors and maximum times, stream buffer peak,
dropped bytes, maximum SD write time. These are sampled and cumulative
since boot, not hardware timing guarantees.

Native regression test exercises the actual audio admission and GPS
snapshot code, cross-thread exclusion, stale release, draining, wrap-safe
GPS age, generation invalidation, and canonical WAV header sizes.
Desktop voice UI runs the existing simulated recorder/assistant; real
I2S/SD timing is not emulated.

## 8F — watch acceptance (pending)

- Rapidly toggle GPS, acquire/loss/reacquire fix and compare UART backlog.
  Run aircraft data while GPS and UI are active; verify same radar center.
- Record, navigate and scroll, then stop; play back saved WAV and check
  file size/header, elapsed time, quality and buffer peak.
- Test SD removal/USB mount contention, repeated start/stop, ten-minute
  cap and slow card. A fault must not hang LVGL or leave a stale lease.
- Test voice capture, swipe away/back, upload and timeout on slow server.
- Test alarm chime while recording, stop and verify audio rail shutdown.
  On a device with the model partition flashed, exercise WakeNet.
- Power off during recording, chime and upload; verify cleanup and
  power-button wake without stale hardware owners.

Phase 9: peripheral power/sleep-resume lifecycle.
