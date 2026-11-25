# Original UI coverage

Inventory: **61** dedicated screen translation units; **61 integrated**, none pending. The earlier estimate of 60 omitted one unit.

Verified at commit `5c6a3405bbdf64a8e3262cbbbd395364134ad99d`:
[Simulator CI run](https://github.com/fredsm/LILYGO-T-WATCH-ULTRA-X-CALMATI/actions/runs/45649991807).
Linux and Windows each pass **69/69** tests; the SDL desktop build passes
**70/70**, including the real SDL render-path smoke test.

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

Additional coverage: original task manager is verified as a bonus entry. Watch face (main.cpp), onboarding and boot overlays are separate from the 61 dedicated screen inventory and are not claimed as covered.
