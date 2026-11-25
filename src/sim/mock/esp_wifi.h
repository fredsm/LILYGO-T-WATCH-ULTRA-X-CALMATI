#pragma once
#include "WiFi.h"
enum {WIFI_IF_STA}; enum wifi_second_chan_t {WIFI_SECOND_CHAN_NONE};
inline int esp_wifi_get_mode(wifi_mode_t* p){*p=WiFi.current;return 0;}
inline int esp_wifi_get_mac(int,uint8_t* p){for(int i=0;i<6;++i)p[i]=i;return 0;}
inline int esp_wifi_get_max_tx_power(int8_t* p){*p=40;return 0;}
inline int esp_wifi_get_channel(uint8_t* p,wifi_second_chan_t* s){*p=6;*s=WIFI_SECOND_CHAN_NONE;return 0;}

// In-memory SDK facade. Never opens a socket or a wireless interface.
enum wifi_promiscuous_pkt_type_t {WIFI_PKT_MGMT,WIFI_PKT_CTRL,WIFI_PKT_DATA};
struct wifi_promiscuous_pkt_t {struct {int sig_len=128; int8_t rssi=-55;} rx_ctrl;};
struct wifi_promiscuous_filter_t {uint32_t filter_mask;};
constexpr uint32_t WIFI_PROMIS_FILTER_MASK_ALL=0xffffffff;
inline void (*sim_promisc_cb)(void*,wifi_promiscuous_pkt_type_t)=nullptr;
inline bool sim_promisc_enabled=false;
inline int esp_wifi_set_promiscuous(bool v){sim_promisc_enabled=v;return 0;}
inline int esp_wifi_set_promiscuous_filter(const wifi_promiscuous_filter_t*){return 0;}
inline int esp_wifi_set_promiscuous_rx_cb(void (*cb)(void*,wifi_promiscuous_pkt_type_t)){sim_promisc_cb=cb;return 0;}
inline int esp_wifi_set_channel(int ch,wifi_second_chan_t){
 if(sim_promisc_enabled && sim_promisc_cb){wifi_promiscuous_pkt_t p; p.rx_ctrl.rssi=-35-ch*3; for(int i=0;i<ch*2;++i)sim_promisc_cb(&p,WIFI_PKT_DATA);} return 0;
}
inline int esp_wifi_stop(){return 0;}
inline int esp_wifi_deinit(){return 0;}
