# Original UI coverage

Inventory: **62** dedicated screen translation units; **62 integrated**, none pending. This includes the 61 original screens and the shared Quick Controls screen.

Verified at commit `270d9f18119637d09249be68659b91b33ef42821`:
[Simulator CI run](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45901764272).
Linux and Windows native builds each pass **77/77** tests; Linux and Windows
SDL desktop builds each pass **79/79**, including the real SDL render-path
smoke test. The [ESP32 firmware validation build](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45901764256) also passes.
[Download the firmware package](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45901764256/artifacts/15155653349).

Every dedicated screen links the original firmware translation unit, opens,
renders, returns to the launcher and reopens. The complete navigation cycle also
passes in a single process. The original task manager is tested separately.
Interaction coverage includes port-scan completion, BLE clear and shared stack
ownership, simulated USB execution, NFC write/read and before/after captures,
GATT enumeration, PNG tile decoding and voice response.

These are automated host checks, not a claim of exhaustive manual visual review
or hardware validation. Navigation and Quick Controls now run in both firmware and simulator; real radio behavior still requires on-device acceptance testing.

| Source | Native status |
|---|---|
| src/adsb_screen.cpp | Headless + SDL smoke verified |
| src/alarm_screen.cpp | Headless + SDL smoke verified |
| src/analyze_screen.cpp | Headless + SDL smoke verified |
| src/aprs_screen.cpp | Headless + SDL smoke verified |
| src/arp_mitm_screen.cpp | Headless + SDL smoke verified |
| src/badusb_screen.cpp | Headless + SDL smoke verified |
| src/ble_pair_screen.cpp | Headless + SDL smoke verified |
| src/bluetooth_screen.cpp | Headless + SDL smoke verified |
| src/bt_analyze_screen.cpp | Headless + SDL smoke verified |
| src/calendar_screen.cpp | Headless + SDL smoke verified |
| src/calmati_remote_screen.cpp | Headless + SDL smoke verified |
| src/calmati_result_screen.cpp | Headless + SDL smoke verified |
| src/calmati_targets_screen.cpp | Headless + SDL smoke verified |
| src/camera_screen.cpp | Headless + SDL smoke verified |
| src/car_screen.cpp | Headless + SDL smoke verified |
| src/channels_screen.cpp | Headless + SDL smoke verified |
| src/configuration_screen.cpp | Headless + SDL smoke verified |
| src/deauther_screen.cpp | Headless + SDL smoke verified |
| src/drone_screen.cpp | Headless + SDL smoke verified |
| src/espnow_screen.cpp | Headless + SDL smoke verified |
| src/evil_twin_screen.cpp | Headless + SDL smoke verified |
| src/gatt_explorer_screen.cpp | Headless + SDL smoke verified |
| src/gps_screen.cpp | Headless + SDL smoke verified |
| src/lora_analyze_screen.cpp | Headless + SDL smoke verified |
| src/lora_screen.cpp | Headless + SDL smoke verified |
| src/map_screen.cpp | Headless + SDL smoke verified |
| src/meshtastic_screen.cpp | Headless + SDL smoke verified |
| src/meta_screen.cpp | Headless + SDL smoke verified |
| src/mic_screen.cpp | Headless + SDL smoke verified |
| src/mifare_screen.cpp | Headless + SDL smoke verified |
| src/mouse_screen.cpp | Headless + SDL smoke verified |
| src/nfc_credit_screen.cpp | Headless + SDL smoke verified |
| src/nfc_screen.cpp | Headless + SDL smoke verified |
| src/nfc_write_screen.cpp | Headless + SDL smoke verified |
| src/nodes_screen.cpp | Headless + SDL smoke verified |
| src/quick_controls_screen.cpp | Headless + SDL smoke verified |
| src/pager_screen.cpp | Headless + SDL smoke verified |
| src/pet_screen.cpp | Headless + SDL smoke verified |
| src/phantom_flood_screen.cpp | Headless + SDL smoke verified |
| src/portscan_screen.cpp | Headless + SDL smoke verified |
| src/printer_screen.cpp | Headless + SDL smoke verified |
| src/rid_spoof_screen.cpp | Headless + SDL smoke verified |
| src/scan_screen.cpp | Headless + SDL smoke verified |
| src/send_message_screen.cpp | Headless + SDL smoke verified |
| src/settings_screen.cpp | Headless + SDL smoke verified |
| src/skimmer_screen.cpp | Headless + SDL smoke verified |
| src/stopwatch_screen.cpp | Headless + SDL smoke verified |
| src/subghz_sentinel_screen.cpp | Headless + SDL smoke verified |
| src/tesla_cp_screen.cpp | Headless + SDL smoke verified |
| src/threat_radar_screen.cpp | Headless + SDL smoke verified |
| src/time_screen.cpp | Headless + SDL smoke verified |
| src/timer_screen.cpp | Headless + SDL smoke verified |
| src/tools_screen.cpp | Headless + SDL smoke verified |
| src/tpms_screen.cpp | Headless + SDL smoke verified |
| src/usb_sd_screen.cpp | Headless + SDL smoke verified |
| src/voice_screen.cpp | Headless + SDL smoke verified |
| src/wardriver_screen.cpp | Headless + SDL smoke verified |
| src/waterfall_screen.cpp | Headless + SDL smoke verified |
| src/webpanel_screen.cpp | Headless + SDL smoke verified |
| src/wifi_radio_screen.cpp | Headless + SDL smoke verified |
| src/wifi_screen.cpp | Headless + SDL smoke verified |
| src/wpa3_sae_screen.cpp | Headless + SDL smoke verified |

Additional coverage: original task manager and the original watch face from
main.cpp are verified outside the dedicated screen inventory. Tests cover
clock face switching, repeated four-direction swipe round trips, Tools tile
scrolling and edge returns, opening the original Stopwatch tile, firmware
Power/Back routes and debounce, UTF-8 compose editing, confirmation, cancellation
and hidden-field rejection. The desktop exposes touchable watch buttons below
the unchanged 410 x 502 viewport.

Phases 4-6 add a separate hardware panel with shared battery, charging,
brightness, display power, GPS fix/satellite count and virtual SD presence.
Default brightness is now 100%. The original skull splash/layout and original
onboarding source are reused; tests verify fade timing, optional acceptance and
session-only consent. SD removal blocks virtual reads/writes without erasing
its in-memory contents.

The SDL end-to-end test starts through the splash and injects mouse, native
finger and keyboard events through the desktop event loop. It checks four-way
swipe round trips, panel battery updates on the original clock, brightness
synchronization, charging/GPS/SD adapters, Power while the display is off,
panel keyboard isolation and close/reopen, catalog access, compose editing,
Escape cancellation and final return to the clock. Premature exit fails the test.

Exhaustive manual touchscreen/visual verification and real hardware behavior
remain outside automated host coverage. See [acceptance checks](TESTING.md)
for the included scripts and hands-on touchscreen checklist.

[Windows package for this verified commit](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45901764272/artifacts/15155813609).

## Simulator customization on dev

The 218 approved translations now live in firmware sources, including clock
weather/countdown wording. Host builds compile the same English UI directly.
The catalog has 63 entries: 62 dedicated screens (including shared Quick Controls)
and Task Manager.

Clock swipe up opens Tools; its first tile is Time. Time opens the original
utility menu and returns to Tools on swipe down or Back. Clock swipe down opens
Quick Controls with Wi-Fi, Bluetooth, GPS, LoRa, NFC, brightness and Settings.
Radio controls reuse the firmware handlers through host adapters. The real
watch uses these same handlers against its hardware.
Brightness cycles 33/66/100%. Settings also remains accessible via Back at home.

Tests cover repeated swipe round trips, Time's first-tile position and touch
activation, the nested Stopwatch path, every quick radio toggled on/off, all
brightness presets, Settings touch activation and Back navigation. The SDL
end-to-end test covers the new routes and Settings shortcut as well.

## Firmware promotion

Clock swipe up now opens Tools with Time first; swipe down opens the shared
Quick Controls screen. Power/Back home shortcuts and left/right gestures are
preserved. Time returns to Tools. Simulator-only routing and tile injection
have been removed so host tests exercise the production routes directly.

Radio tiles read actual Wi-Fi/Bluetooth state, GPS power, shared LoRa usage and
NFC power. Tests cover externally enabled Wi-Fi and ownership retained by BLE
and pager consumers. Brightness presets update Settings and save to writable
SD; dim/wake restores the selected level. Firmware CI compiles the ESP32 target;
physical touch, radio operation and dim/wake still need on-watch testing.

English conversion covers 24 source files. Regression checks verify all 218
translated literals in the firmware, with unchanged non-string C++ code.
