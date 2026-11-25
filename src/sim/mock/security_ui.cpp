// These state machines only drive UI fixtures. They never transmit, connect,
// capture credentials, inject input, or access a physical card.
#include "../../arp_mitm.h"
#include "../../badusb.h"
#include "../../deauther.h"
#include "../../wpa3_sae.h"
#include "../../evil_portal.h"
#include "../../evil_twin_verify.h"
#include "../../mifare.h"
#include "../../scan_engine.h"
#include <lvgl.h>
#include <cstdio>
#include <cstring>
namespace {
 bool arp=false,sweep=false,usb=false,deauth=false,sae=false,portal=false,card=false,dict=false,scanner=false;
 int hosts=0,devices=0;uint32_t usb_start=0,dict_start=0,arp_start=0,deauth_start=0,sae_start=0;
 char portal_name[33]="SIM AP";
 int progress(uint32_t start){return int((lv_tick_get()-start)/20);}
}
ArpNet arp_mitm_prepare(){return ARP_NET_CONNECTED;}
ArpNet arp_mitm_poll_net(){return ARP_NET_CONNECTED;}
void arp_mitm_net_info(char* out,int n){std::snprintf(out,n,"SIM LAN 192.0.2.10 gw .1");}
void arp_mitm_request_sweep(){hosts=2;sweep=true;}
bool arp_mitm_sweeping(){return false;}
bool arp_mitm_sweep_done(){return sweep;}
int arp_mitm_host_count(){return hosts;}
bool arp_mitm_host(int i,uint8_t* ip,uint8_t mac[6],bool* gw){if(i<0||i>=hosts)return false;*ip=i?20:1;std::memset(mac,0,6);mac[0]=2;mac[5]=*ip;*gw=i==0;return true;}
void arp_mitm_prefix(uint8_t out[3]){out[0]=192;out[1]=0;out[2]=2;}
bool arp_mitm_start(int target){if(target!=ARP_TARGET_ALL&&(target<0||target>=hosts))return false;arp=true;arp_start=lv_tick_get();return true;}
void arp_mitm_stop(){arp=false;}
bool arp_mitm_is_running(){return arp;}
uint32_t arp_mitm_cycles(){return arp?(lv_tick_get()-arp_start)/100:0;}
void badusb_begin(){}
bool badusb_ready(){return true;}
bool badusb_run(const char* script){if(usb||!script||!*script)return false;usb=true;usb_start=lv_tick_get();return true;}
bool badusb_is_running(){if(usb&&progress(usb_start)>=100)usb=false;return usb;}
int badusb_progress(){int p=progress(usb_start);return p>100?100:p;}
void badusb_stop(){usb=false;}
bool deauther_start(const uint8_t[6],uint8_t){if(deauth)return false;deauth=true;deauth_start=lv_tick_get();return true;}
bool deauther_start_client(const uint8_t[6],const uint8_t b[6],uint8_t c){return deauther_start(b,c);}
void deauther_stop(){deauth=false;}
bool deauther_is_running(){return deauth;}
uint32_t deauther_frames_sent(){return deauth?(lv_tick_get()-deauth_start)/10:0;}
bool wpa3_sae_start(const uint8_t[6],uint8_t){if(sae)return false;sae=true;sae_start=lv_tick_get();return true;}
void wpa3_sae_stop(){sae=false;}
bool wpa3_sae_is_running(){return sae;}
uint32_t wpa3_sae_frames_sent(){return sae?(lv_tick_get()-sae_start)/10:0;}
bool evil_portal_start(const char* name,const uint8_t[6],uint8_t){portal=true;std::snprintf(portal_name,sizeof(portal_name),"%s",name);return true;}
void evil_portal_stop(){portal=false;}
bool evil_portal_running(){return portal;}
void evil_portal_tick(){}
int evil_portal_client_count(){return portal?1:0;}
int evil_portal_cred_count(){return portal?1:0;}
const char* evil_portal_last_cred(){return portal?"SIM-ONLY-PASSWORD":"";}
const char* evil_portal_ssid(){return portal_name;}
int evil_portal_get_creds(EvilCred* out,int max){if(!portal||!out||max<1)return 0;*out={1,"12:00:00","192.0.2.20","SIM-ONLY-PASSWORD"};return 1;}
EvilVerifyStatus evil_verify_status(int){return EV_VERIFY_OK;}
void mifare_reader_start(){card=true;}
void mifare_reader_stop(){card=false;dict=false;}
void mifare_reader_worker(){}
bool mifare_have_card(){return card;}
bool mifare_get_card(MifareCard* out){if(!card||!out)return false;*out={{2,0,0,1},4,8,{4,0},"SIM Classic 1K",16,true};return true;}
void mifare_forget_card(){card=false;}
bool mifare_dict_start(){if(!card||dict)return false;dict=true;dict_start=lv_tick_get();return true;}
void mifare_dict_stop(){dict=false;}
bool mifare_dict_running(){if(dict&&progress(dict_start)>=100)dict=false;return dict;}
int mifare_dict_progress(){int p=progress(dict_start);return p>100?100:p;}
int mifare_dict_found(){return card?1:0;}
int mifare_dict_total(){return 16;}
bool mifare_dict_key(int s,uint8_t* type,uint8_t key[6]){if(!card||s!=0)return false;*type=0;std::memset(key,0xff,6);return true;}
void scan_engine_start(){scanner=true;devices=2;}
void scan_engine_stop(){scanner=false;}
bool scan_engine_running(){return scanner;}
void scan_engine_tick(){}
void scan_engine_reset(){devices=0;}
void scan_engine_stats(ScanStats* out){*out={};if(devices){out->wifi=1;out->ble=1;out->threats=1;out->per_cat[TR_CAT_AIRTAG]=1;out->total_seen=2;}}
int scan_engine_get_devices(ScanDev* out,int max){if(!out||max<1||!devices)return 0;*out={{2,0,0,0,0,2},-50,-45,SCAN_KIND_BLE,TR_CAT_AIRTAG,4,lv_tick_get(),"SIM tracker"};return 1;}
bool scan_engine_wifi_phase(){return (lv_tick_get()/1000)%2==0;}
void threatradar_observe(const uint8_t*,int8_t,uint8_t){}
