#include <vector>
#include <algorithm>
#include "../../pingsweep.h"
#include "../../hostresolve.h"
#include "../../wifi_creds.h"
#include "../../ble_scan_manager.h"
#include "../../meta_glasses.h"
#include "esp_bt.h"
#include "WiFi.h"
#include <cstring>
namespace {bool auto_connect=false,meta=false,capture=false;int hits=0,devices=0;}
void tools_screen_show();
void pingsweep_start(){devices=1;}
void pingsweep_stop(){}
bool pingsweep_is_running(){return false;}
int pingsweep_scanned(){return devices;}
int pingsweep_total(){return 1;}
int pingsweep_device_count(){return devices;}
const PingDevice* pingsweep_device(int i){static PingDevice d={0xc000020a,2,{},false,"SIM host",PNAME_PTR};return i==0&&devices?&d:nullptr;}
bool hostresolve_is_running(){return false;}
void hostresolve_get_stats(HostResolveStats* p){*p={};}
bool wifi_creds_pass_for(const char*,char*,size_t){return false;}
void wifi_creds_save(const char*,const char*){}
bool wifi_creds_autoconnect(){if(auto_connect)WiFi.begin("SIM");return auto_connect;}
bool wifi_creds_autoconnect_enabled(){return auto_connect;}
void wifi_creds_set_autoconnect(bool v){auto_connect=v;}
namespace { std::vector<ble_scan_cb_t> ble_consumers; }
bool ble_scan_add(ble_scan_cb_t cb){
 if(std::find(ble_consumers.begin(),ble_consumers.end(),cb)==ble_consumers.end())ble_consumers.push_back(cb);
 sim_ble_active=true;
 esp_ble_gap_cb_param_t p{};
 // Synthetic Remote ID advert consumed by the original drone decoder.
 p.scan_rst.adv_data_len=30;p.scan_rst.ble_adv[0]=29;p.scan_rst.ble_adv[1]=0x16;
 p.scan_rst.ble_adv[2]=0xfa;p.scan_rst.ble_adv[3]=0xff;p.scan_rst.ble_adv[4]=0x0d;
 p.scan_rst.ble_adv[5]=2;p.scan_rst.ble_adv[6]=0x21;
 std::memcpy(p.scan_rst.ble_adv+7,"SIM-DRONE-0001",14);cb(&p);return true;
}
void ble_scan_remove(ble_scan_cb_t cb){ble_consumers.erase(std::remove(ble_consumers.begin(),ble_consumers.end(),cb),ble_consumers.end());sim_ble_active=!ble_consumers.empty();}
bool ble_scan_active(){return sim_ble_active;}
int ble_scan_consumer_count(){return int(ble_consumers.size());}
bool phone_link_connected(){return false;}
bool meta_glasses_start(){meta=true;return true;}
void meta_glasses_stop(){meta=false;}
int meta_glasses_count(){return 0;}
int meta_glasses_get(MetaHit*,int){return 0;}
void meta_glasses_reset(){hits=0;}
void meta_glasses_set_capture(bool v){capture=v;}
bool meta_glasses_capture_enabled(){return capture;}
bool meta_glasses_save_labeled(const uint8_t*,const char*){return false;}
int meta_glasses_capture_total(){return hits;}
void clock_screen_set_analog_face(bool){}
void clock_screen_set_12h(bool){}
void clock_screen_set_matrix(bool){}
void clock_screen_set_show_day(bool){}
void clock_screen_set_show_date(bool){}
void clock_screen_set_show_ampm(bool){}
void clock_screen_set_show_secs(bool){}
void clock_screen_set_vibrate(bool){}
void clock_screen_set_motion_wake(bool){}
void clock_screen_set_manual_override(bool){}
void clock_screen_set_dim_timeout(uint32_t){}
void clock_screen_set_dim_brightness(uint8_t){}
void clock_screen_apply_manual_time(int,int,int,int,int){}

namespace {int ble_holds=0;}
bool ble_stack_acquire(){++ble_holds;sim_ble_active=true;return true;}
void ble_stack_release(){if(ble_holds)--ble_holds;sim_ble_active=ble_holds||!ble_consumers.empty();}
void ble_gap_forward_set(ble_gap_forward_t){}

bool meta_glasses_is_running(){return meta;}
