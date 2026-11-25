// Synthetic observations shared by the original camera, wardriver, pet and tools UIs.
#include "../../wifi_beacon_manager.h"
#include "../../cam_audit.h"
#include "../../flock.h"
#include "../../airtag.h"
#include "../../flipper.h"
#include "../../skimmer.h"
#include "../../skimmer_probe.h"
#include "../../evil_twin.h"
#include "../../handshake.h"
#include "../../counter_tail.h"
#include "../../pwnagotchi_peer.h"
#include "../../stealth.h"
#include <vector>
#include <algorithm>
#include <cstring>
#include <cstdio>
namespace {
 std::vector<wifi_beacon_cb_t> consumers;
 bool camera=false,cam_queued=false,probe=false,armed=false;
 bool flock=false,airtag=false,flipper=false,skimmer=false,twin=false,handshake=false;
}
bool wifi_beacon_add(wifi_beacon_cb_t cb){
 if(std::find(consumers.begin(),consumers.end(),cb)==consumers.end())consumers.push_back(cb);
 WifiBeacon b={{2,0,0,0,0,1},"SIM camera","[WPA2-PSK][ESS]",-48,6};cb(&b);return true;
}
void wifi_beacon_remove(wifi_beacon_cb_t cb){consumers.erase(std::remove(consumers.begin(),consumers.end(),cb),consumers.end());}
bool wifi_beacon_active(){return !consumers.empty();}
void wifi_beacon_set_data_capture(bool){}
bool cam_audit_start(){camera=true;cam_queued=true;return true;}
void cam_audit_stop(){camera=false;cam_queued=false;}
bool cam_audit_is_running(){return camera&&cam_queued;}
CamAuditPhase cam_audit_phase(){return camera?(cam_queued?CA_PROBE:CA_DONE):CA_IDLE;}
int cam_audit_scanned(){return camera&&!cam_queued?1:0;}
int cam_audit_total(){return 1;}
bool cam_audit_next_finding(CamFinding* out){if(!cam_queued||!out)return false;*out={0xc0000214,{2,0,0,0,0,20},true,"SIM camera",CL_RTSP_AUTH,554,"SIM protected stream"};cam_queued=false;return true;}
const char* flock_classify(const uint8_t*,const char* name){return name&&std::strstr(name,"SIM")?"SIM camera":nullptr;}
bool flock_check(const uint8_t*,int8_t,const char*,char){return true;}
bool airtag_check(const uint8_t*,int8_t,uint8_t,const uint8_t*,int){return false;}
bool flipper_check(const uint8_t*,int8_t,uint8_t,const uint8_t*,int){return false;}
bool skimmer_check(const uint8_t*,int8_t,uint8_t,const uint8_t*,int){return true;}
bool evil_twin_check(const uint8_t*,const char*,const char*,int8_t,uint8_t){return false;}
bool skimmer_ad_match(const uint8_t*,int n,char* name,int size){if(n<=0)return false;std::snprintf(name,size,"SIM serial module");return true;}
void skimmer_probe_start(const uint8_t*,esp_ble_addr_type_t){probe=true;}
bool skimmer_probe_busy(){return false;}
bool skimmer_probe_done(){return probe;}
const char* skimmer_probe_result(){return "SIM fixture\nManufacturer: Simulator\nSerial service: present\nNo physical device contacted";}
#define DETECTOR(name,state) bool name##_start(){state=true;return true;} void name##_stop(){state=false;} bool name##_is_running(){return state;} int name##_get_count(){return state?1:0;} void name##_reset_count(){} void name##_bg_tick(){}
DETECTOR(flock,flock)
DETECTOR(airtag,airtag)
DETECTOR(flipper,flipper)
DETECTOR(skimmer,skimmer)
DETECTOR(evil_twin,twin)
#undef DETECTOR
bool handshake_start(){handshake=true;return true;}
void handshake_stop(){handshake=false;}
bool handshake_is_running(){return handshake;}
int handshake_pwnd_count(){return handshake?1:0;}
void counter_tail_observe_wifi(const uint8_t*,int8_t){}
void counter_tail_observe_ble(const uint8_t*,int8_t){}
int pwnagotchi_peer_count(){return 1;}
const char* pwnagotchi_last_name(){return "SIM peer";}
int pwnagotchi_last_pwnd(){return 1;}
void stealth_set_armed(bool v){armed=v;}
bool stealth_armed(){return armed;}
bool stealth_active(){return false;}
