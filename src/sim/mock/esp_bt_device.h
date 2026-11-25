#pragma once
#include <cstdint>
inline const uint8_t* esp_bt_dev_get_address(){static uint8_t m[6]={2,0,0,0,0,1};return m;}
