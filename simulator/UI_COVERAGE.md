# Original UI coverage

Inventory: **61** dedicated screen translation units; **40 integrated**, 21 pending. The earlier estimate of 60 omitted one unit.

Inventory of dedicated screen translation units. Integrated means included in native build; verified means headless CI passed. Desktop interaction remains unverified for all. Additional UI in main.cpp, onboarding and other modules must be inventoried separately. No UI redesign before full coverage.

| Source | Native status |
|---|---|
| src/adsb_screen.cpp | Integrated, unverified |
| src/alarm_screen.cpp | Integrated, unverified |
| src/analyze_screen.cpp | Integrated; CI pending |
| src/aprs_screen.cpp | Integrated, unverified |
| src/arp_mitm_screen.cpp | Pending |
| src/badusb_screen.cpp | Pending |
| src/ble_pair_screen.cpp | Integrated; CI pending |
| src/bluetooth_screen.cpp | Integrated, unverified |
| src/bt_analyze_screen.cpp | Integrated; CI pending |
| src/calendar_screen.cpp | Integrated, unverified |
| src/calmati_remote_screen.cpp | Integrated, unverified |
| src/calmati_result_screen.cpp | Integrated, unverified |
| src/calmati_targets_screen.cpp | Integrated, unverified |
| src/camera_screen.cpp | Pending |
| src/car_screen.cpp | Integrated, unverified |
| src/channels_screen.cpp | Integrated, unverified |
| src/configuration_screen.cpp | Integrated, unverified |
| src/deauther_screen.cpp | Pending |
| src/drone_screen.cpp | Pending |
| src/espnow_screen.cpp | Integrated, unverified |
| src/evil_twin_screen.cpp | Pending |
| src/gatt_explorer_screen.cpp | Pending |
| src/gps_screen.cpp | Integrated, unverified |
| src/lora_analyze_screen.cpp | Integrated; CI pending |
| src/lora_screen.cpp | Integrated, unverified |
| src/map_screen.cpp | Pending |
| src/meshtastic_screen.cpp | Integrated, unverified |
| src/meta_screen.cpp | Integrated, unverified |
| src/mic_screen.cpp | Integrated, unverified |
| src/mifare_screen.cpp | Pending |
| src/mouse_screen.cpp | Integrated, unverified |
| src/nfc_credit_screen.cpp | Pending |
| src/nfc_screen.cpp | Pending |
| src/nfc_write_screen.cpp | Pending |
| src/nodes_screen.cpp | Integrated, unverified |
| src/pager_screen.cpp | Integrated, unverified |
| src/pet_screen.cpp | Pending |
| src/phantom_flood_screen.cpp | Pending |
| src/portscan_screen.cpp | Integrated; CI pending |
| src/printer_screen.cpp | Integrated, unverified |
| src/rid_spoof_screen.cpp | Pending |
| src/scan_screen.cpp | Pending |
| src/send_message_screen.cpp | Integrated, unverified |
| src/settings_screen.cpp | Integrated, unverified |
| src/skimmer_screen.cpp | Pending |
| src/stopwatch_screen.cpp | Headless verified |
| src/subghz_sentinel_screen.cpp | Integrated; CI pending |
| src/tesla_cp_screen.cpp | Integrated; CI pending |
| src/threat_radar_screen.cpp | Integrated; CI pending |
| src/time_screen.cpp | Integrated, unverified |
| src/timer_screen.cpp | Integrated, unverified |
| src/tools_screen.cpp | Pending |
| src/tpms_screen.cpp | Integrated, unverified |
| src/usb_sd_screen.cpp | Integrated, unverified |
| src/voice_screen.cpp | Pending |
| src/wardriver_screen.cpp | Pending |
| src/waterfall_screen.cpp | Integrated; CI pending |
| src/webpanel_screen.cpp | Integrated, unverified |
| src/wifi_radio_screen.cpp | Integrated, unverified |
| src/wifi_screen.cpp | Integrated, unverified |
| src/wpa3_sae_screen.cpp | Pending |

Additional coverage: watch face (main.cpp), onboarding, task manager, overlays and dialogs: pending.
