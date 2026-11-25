// Host-only, deterministic fixtures. No real network, BLE, radio or phone I/O.
#include "../../portscan.h"
#include "../../ble_pair_audit.h"
#include "../../threat_radar.h"
#include "../../tesla_cp.h"
#include "../../phone_link.h"
#include <lvgl.h>
#include <cstdio>
namespace {
 bool scanning=false,finished=false,pairing=false,phone=false; int pair_count=0;
 uint32_t target=0,started=0; PortScanTech technique=PSTECH_TCP_CONNECT;
 PortScanPreset preset=PSPRE_TOP_20; uint16_t low=1,high=1024;
 int result_count=0; bool threats=true;
 PortScanResult results[]={{80,PSTATE_OPEN,0,"SIM HTTP service"},{443,PSTATE_OPEN,0,"SIM TLS service"}};
 void update_scan(){if(scanning && lv_tick_get()-started>=600){scanning=false;finished=true;result_count=2;}}
}
bool portscan_start(uint32_t ip,PortScanTech tech,PortScanPreset pre,uint16_t lo,uint16_t hi){
 if(scanning || !ip || !tech || (pre==PSPRE_CUSTOM && (!lo || lo>hi)))return false;
 target=ip;technique=tech;preset=pre;low=lo;high=hi;started=lv_tick_get();scanning=true;finished=false;result_count=0;return true;
}
void portscan_stop(){scanning=false;}
bool portscan_is_running(){update_scan();return scanning;}
bool portscan_just_finished(){update_scan();bool v=finished;finished=false;return v;}
int portscan_scanned(){update_scan();return result_count;}
int portscan_total(){return 2;}
int portscan_result_count(){update_scan();return result_count;}
const PortScanResult* portscan_result(int i){return i>=0&&i<result_count?&results[i]:nullptr;}
void portscan_clear_results(){result_count=0;}
uint32_t portscan_target_ip(){return target;}
PortScanTech portscan_tech(){return technique;}
PortScanPreset portscan_preset(){return preset;}
uint16_t portscan_custom_lo(){return low;}
uint16_t portscan_custom_hi(){return high;}
void portscan_poll(){update_scan();}
bool bpa_start(){pairing=true;pair_count=2;return true;}
void bpa_stop(){pairing=false;}
bool bpa_is_running(){return pairing;}
void bpa_clear(){pair_count=0;}
int bpa_count(){return pair_count;}
bool bpa_get(int i,BpaDevice* out){
 if(!out||i<0||i>=pair_count)return false;
 *out={};out->mac[0]=2;out->mac[5]=uint8_t(i+1);out->rssi=-45-i*20;
 std::snprintf(out->name,sizeof(out->name),"SIM peripheral %d",i+1);
 out->addr=i?BPA_ADDR_RPA:BPA_ADDR_PUBLIC;out->connectable=true;
 out->risk=i?BPA_RISK_LOW:BPA_RISK_HIGH;out->hid=!i;out->limited_disc=!i;return true;
}
const char* bpa_tags(const BpaDevice* d,char* out,int n){std::snprintf(out,n,"SIM - %s",d->hid?"PAIRING - HID - public":"private address");return out;}
int threatradar_threat_count(){return threats?1:0;}
int threatradar_top_level(){return threats?TR_LVL_LIKELY:TR_LVL_NONE;}
int threatradar_get_threats(TrThreat* out,int max){
 if(!threats||!out||max<1)return 0;
 *out={};out->mac[0]=2;out->mac[5]=1;out->category=TR_CAT_AIRTAG;out->level=TR_LVL_LIKELY;
 out->waypoints=4;out->span_m=450;out->span_min=12;out->best_rssi=-51;out->active=true;
 std::snprintf(out->first_time,sizeof(out->first_time),"12:00");return 1;
}
void threatradar_reset(){threats=false;}
const char* threatradar_category_name(uint8_t){return "SIM tracker";}
const char* threatradar_level_name(uint8_t){return "Simulated follow";}
bool phone_link_start(){phone=true;return true;}
void phone_link_stop(){phone=false;}
bool phone_link_active(){return phone;}
bool tesla_cp_transmit_us(){return true;}
bool tesla_cp_transmit_eu(){return true;}
int16_t tesla_cp_last_error(){return 0;}
