#pragma once
#include <cstdint>
constexpr int ESP_UUID_LEN_16=2,ESP_UUID_LEN_32=4,ESP_GATT_OK=0,ESP_GATT_DB_CHARACTERISTIC=1;
constexpr int ESP_GATT_CHAR_PROP_BIT_READ=2,ESP_GATT_CHAR_PROP_BIT_WRITE=8,ESP_GATT_CHAR_PROP_BIT_NOTIFY=16;
using esp_gatt_if_t=int;
constexpr int ESP_GATT_IF_NONE=-1;
struct esp_bt_uuid_t {int len=ESP_UUID_LEN_16;union U{uint16_t uuid16;uint32_t uuid32;uint8_t uuid128[16];U():uuid16(0x180f){}}uuid;};
struct esp_gattc_char_elem_t {esp_bt_uuid_t uuid;uint8_t properties=ESP_GATT_CHAR_PROP_BIT_READ;};
