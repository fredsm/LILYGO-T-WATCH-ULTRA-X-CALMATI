#pragma once
#include <cstdint>
enum {ESP_BT_CONTROLLER_STATUS_IDLE,ESP_BT_CONTROLLER_STATUS_INITED,ESP_BT_CONTROLLER_STATUS_ENABLED};
inline bool sim_ble_active=false;
inline int esp_bt_controller_get_status(){return sim_ble_active?ESP_BT_CONTROLLER_STATUS_ENABLED:ESP_BT_CONTROLLER_STATUS_IDLE;}
using esp_power_level_t=int;
enum {ESP_PWR_LVL_N0=4,ESP_BLE_PWR_TYPE_DEFAULT=0};
inline int esp_ble_tx_power_get(int){return ESP_PWR_LVL_N0;}
