# Original UI coverage

Inventory: **61** dedicated screen translation units; **61 integrated**, none pending. The earlier estimate of 60 omitted one unit.

Verified at commit `766e34e9f8886292a8b3da16a3486934f2d9933a`:
[Simulator CI run](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45712681606).
Linux and Windows native builds each pass **73/73** tests; Linux and Windows
SDL desktop builds each pass **74/74**, including the real SDL render-path
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
the unchanged 410 x 502 viewport. Onboarding, boot overlays and exhaustive
manual touchscreen/visual verification remain outside this coverage.

[Windows package for this verified commit](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45712681606/artifacts/15069465546).
