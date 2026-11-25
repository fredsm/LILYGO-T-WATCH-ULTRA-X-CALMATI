# Native mock boundary

watch.hpp models deterministic watch time, RTC, battery, charging, brightness, display power, button edges, motion and haptic events.

storage.hpp models in-memory SD and namespaced NVS state. Retain Media across simulated reboots; create a fresh Storage controller to reset transient faults. SD supports binary strings, capacity, removal, read-only mode and exclusive USB ownership. No host filesystem access. NVS currently stores strings only. Persistence across process exit, FAT directory semantics, and firmware adapters are not implemented yet.

All mocks are host-only and excluded from firmware. No real radio or network operations.
