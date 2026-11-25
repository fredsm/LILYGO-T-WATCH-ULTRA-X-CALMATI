#pragma once
#include "WiFi.h"
enum {WIFI_IF_STA}; enum wifi_second_chan_t {WIFI_SECOND_CHAN_NONE};
inline int esp_wifi_get_mode(wifi_mode_t* p){*p=WiFi.current;return 0;}
inline int esp_wifi_get_mac(int,uint8_t* p){for(int i=0;i<6;++i)p[i]=i;return 0;}
inline int esp_wifi_get_max_tx_power(int8_t* p){*p=40;return 0;}
inline int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t* s){*p=6;*s=WIFI_SECOND_CHAN_NONE;return 0;}
