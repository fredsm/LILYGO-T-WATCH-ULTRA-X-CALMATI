# Original UI coverage

Inventory: **61** dedicated screen translation units; **61 integrated**, none pending. The earlier estimate of 60 omitted one unit.

Verified at commit `40970b04c3d4cc71fa94257406f3590ba3933770`:
[Simulator CI run](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45755718676).
Linux and Windows native builds each pass **74/74** tests; Linux and Windows
SDL desktop builds each pass **76/76**, including the real SDL render-path
smoke test. The production firmware validation build also passes.

Every dedicated screen links the original firmware translation unit, opens,
renders, returns to the launcher and reopens. The complete navigation cycle also
passes in a single process. The original task manager is tested separately.
Interaction coverage includes port-scan completion, BLE clear and shared stack
ownership, simulated USB execution, NFC write/read and before/after captures,
GATT enumeration, PNG tile decoding and voice response.

These are automated host checks, not a claim of exhaustive manual visual review
or hardware validation. No UI redesign or production hardware changes were made.

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
main.cpp are verified outside the 61 dedicated screen inventory. Tests cover
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

[Windows package for this verified commit](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45755718676/artifacts/15088000862).
