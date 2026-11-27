# Phase 7 — network and radio coordination

## 7A — ownership audit

WiFi and BLE share the ESP32-S3 radio and scarce internal heap. The
existing BLE startup code explicitly frees WiFi before allocating the
Bluetooth host; previously this could race with weather, time sync or
aircraft HTTP work on core 0. SX1262 has a different RF controller but
shares the Phase 4 SPI bus. SPI transaction ownership does not imply
ownership of an entire radio session.

## 7B — controller lifecycle

`radio_manager` serializes controller lifecycle changes with a recursive
mutex (20 ms admission budget) and rejects initialization of the opposite
controller while the first is still allocated. Linker wrappers cover the
WiFi init/start/stop/deinit, mode/config, connection/disconnection calls and
Bluetooth controller/Bluedroid init/enable/disable/deinit. They do not
change packets, scanning algorithms, callbacks or RF parameters. The
existing BLE startup sequence now reserves the transition for its duration
and refuses to tear down WiFi during a managed network job. Partial BLE
initialization is rolled back if the sequence originally owned the
controller.

Locks cover controller calls only; ordinary HTTP and NTP must never hold
the lifecycle mutex while waiting on the network. The timeout is for
admission, **not** a guarantee that an already-admitted vendor call returns
within 20 ms. Native startup/teardown latency remains a hardware test
target; not all legacy radio paths are asynchronous.

## 7C — managed background jobs

- Weather uses a single exclusive lease, borrows an already-connected
  station and tears down only a temporary connection it created. A busy
  or disconnected active radio is reported instead of being reconfigured.
- Time sync handles GOT_IP through the existing notification worker;
  it skips a request if a different job owns the connection (particularly
  a temporary weather connection). Existing UI result delivery is retained.
- Aircraft data uses a cancellable worker lease, with credentials copied
  before task creation, checked spawn outcome, and owner-only WiFi cleanup.
  Its lease lasts until the aircraft session stops. While it is active,
  another managed network request reports busy; stop it to release the radio.
- Normal WiFi control and connection UI checks admission and reconciles
  the displayed radio state; no new settings or mock screens.

A managed job cannot prevent every unwrapped legacy RF operation, and
not all existing clients use a lease. Audio upload and GPS ownership are
follow-up work in Phase 8; complete peripheral sleep/resume is Phase 9.

## 7D — SX1262 and power

Keep the Phase 4 SPI transaction arbiter. The ordinary mesh power toggle
does not cut the rail while another SX1262 user is active; mesh activation
releases the existing analyzer session before the existing handoff, and
APRS will not claim the controller while the analyzer owns it. This is
a narrow lifecycle fix, not a rewrite of all SX1262 session state machines.

Deep sleep closes managed job admission, requests aircraft stop and
defers the final shutdown until managed jobs release their resources.
Weather stops its association wait when draining; time sync skips the
geolocation phase if draining. In-flight HTTP/NTP calls retain their
existing timeouts and must return before shutdown continues. Light sleep
is refused during a managed job. Full rail lifecycle/resume remains
Phase 9.

## 7E — diagnostics and regression tests

System Diagnostics now shows job owner, age, suspend/drain state,
completed jobs, rejected actions, transition lock timeouts, driver
failures, call counts and maximum WiFi/BLE controller call duration.
A driver error can include a benign already-stopped/uninitialized return;
it is not automatically proof of failure of an entire session.

Native tests use the production admission policy, checking cross-stack
exclusion, recursive locking, cross-thread lease rejection, owner-only
release, bounded lock admission, draining/resume, and metrics.
The desktop WiFi mock now returns a success bool from mode(), matching
Arduino's API. No hardware timing is simulated.

## 7F — physical validation pending

- Connect to WiFi, fetch weather, verify the existing station remains
  connected; repeat with WiFi off, confirm temporary radio is cleaned up.
- Open aircraft data, request another managed job, then stop aircraft
  data; verify busy status and eventual release. Exercise no-credentials,
  disconnected network and slow server paths.
- Rapidly switch ordinary WiFi and BLE radio modes; inspect rejected
  transitions, driver errors, largest internal heap block and UI gaps.
- Request 5-second power-off during network work, verify drain before
  deep sleep and correct power-key wake.
- Switch mesh / APRS / analyzer modes and verify rail state stays valid.

Phase 8: GPS and audio subsystem scheduling.
