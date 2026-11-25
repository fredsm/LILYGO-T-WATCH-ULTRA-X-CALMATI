#pragma once
#include <cstdint>
using esp_bd_addr_t=uint8_t[6];
using esp_gap_ble_cb_event_t=int;
struct esp_ble_gap_cb_param_t {
 struct {esp_bd_addr_t bda{2,0,0,0,0,1}; int rssi=-52; uint8_t ble_adv[62]{}; uint8_t adv_data_len=0,scan_rsp_len=0,ble_addr_type=0,ble_evt_type=0; int search_evt=0;} scan_rst;
};
