# Native mock boundary

Host-only deterministic watch state lives in watch.hpp: monotonic time, RTC, battery, charging, brightness, display power, button edges, motion and haptic event recording/failure injection. No real hardware or network calls. Battery and motion are explicitly controlled by tests; no automatic discharge model. RTC is UTC epoch seconds. Brightness is 0–255. This is a native state model, not yet a LilyGoLib adapter. Storage and radio mocks are deferred. Firmware excludes src/sim.
