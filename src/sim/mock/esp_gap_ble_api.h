#pragma once
#include <cstdint>
using esp_bd_addr_t=uint8_t[6];
using esp_gap_ble_cb_event_t=int;
struct esp_ble_gap_cb_param_t {
 struct {esp_bd_addr_t bda{2,0,0,0,0,1}; int rssi=-52; uint8_t ble_adv[62]{}; uint8_t adv_data_len=0,scan_rsp_len=0,ble_addr_type=0,ble_evt_type=0; int search_evt=0;} scan_rst;
};

using esp_ble_addr_type_t=uint8_t;
constexpr int ADV_TYPE_NONCONN_IND=3,BLE_ADDR_TYPE_RANDOM=1,ADV_CHNL_ALL=7,ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY=0;
struct esp_ble_adv_params_t {int adv_int_min,adv_int_max,adv_type,own_addr_type,channel_map,adv_filter_policy;};
inline bool sim_advertising=false;
inline int esp_ble_gap_stop_advertising(){sim_advertising=false;return 0;}
inline int esp_ble_gap_set_rand_addr(const uint8_t*){return 0;}
inline int esp_ble_gap_config_adv_data_raw(const uint8_t*,int){return 0;}
inline int esp_ble_gap_start_advertising(const esp_ble_adv_params_t*){sim_advertising=true;return 0;}
