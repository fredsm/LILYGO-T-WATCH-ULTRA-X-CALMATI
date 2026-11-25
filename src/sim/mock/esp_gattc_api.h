#pragma once
#include "esp_gatt_defs.h"
#include "esp_gap_ble_api.h"
enum esp_gattc_cb_event_t {ESP_GATTC_REG_EVT,ESP_GATTC_OPEN_EVT,ESP_GATTC_CFG_MTU_EVT,ESP_GATTC_SEARCH_RES_EVT,ESP_GATTC_SEARCH_CMPL_EVT,ESP_GATTC_DISCONNECT_EVT};
struct esp_ble_gattc_cb_param_t {
 struct {int status=ESP_GATT_OK;} reg;
 struct {int status=ESP_GATT_OK;uint16_t conn_id=1;} open;
 struct {uint16_t conn_id=1;} cfg_mtu,search_cmpl;
 struct {struct {esp_bt_uuid_t uuid;} srvc_id;uint16_t start_handle=1,end_handle=4,conn_id=1;} search_res;
};
inline void (*sim_gatt_cb)(esp_gattc_cb_event_t,esp_gatt_if_t,esp_ble_gattc_cb_param_t*)=nullptr;
inline void sim_gatt_emit(esp_gattc_cb_event_t event){if(sim_gatt_cb){esp_ble_gattc_cb_param_t p;sim_gatt_cb(event,1,&p);}}
inline int esp_ble_gattc_register_callback(decltype(sim_gatt_cb) cb){sim_gatt_cb=cb;return 0;}
inline int esp_ble_gattc_app_register(int){sim_gatt_emit(ESP_GATTC_REG_EVT);return 0;}
inline int esp_ble_gattc_app_unregister(int){return 0;}
inline int esp_ble_gattc_open(int,const uint8_t*,esp_ble_addr_type_t,bool){sim_gatt_emit(ESP_GATTC_OPEN_EVT);return 0;}
inline int esp_ble_gattc_send_mtu_req(int,uint16_t){sim_gatt_emit(ESP_GATTC_CFG_MTU_EVT);return 0;}
inline int esp_ble_gattc_search_service(int,uint16_t,void*){sim_gatt_emit(ESP_GATTC_SEARCH_RES_EVT);sim_gatt_emit(ESP_GATTC_SEARCH_CMPL_EVT);return 0;}
inline int esp_ble_gattc_get_attr_count(int,uint16_t,int,uint16_t,uint16_t,int,uint16_t* count){*count=1;return 0;}
inline int esp_ble_gattc_get_all_char(int,uint16_t,uint16_t,uint16_t,esp_gattc_char_elem_t* out,uint16_t* count,int){if(*count){*count=1;*out={};out->uuid.uuid.uuid16=0x2a19;}return 0;}
inline int esp_ble_gattc_close(int,uint16_t){sim_gatt_emit(ESP_GATTC_DISCONNECT_EVT);return 0;}
