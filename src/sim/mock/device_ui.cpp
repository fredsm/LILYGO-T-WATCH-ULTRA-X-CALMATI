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
void portscan_screen_show(uint32_t,const char*){tools_screen_show();}
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
bool ble_scan_add(ble_scan_cb_t){sim_ble_active=true;return true;}
void ble_scan_remove(ble_scan_cb_t){sim_ble_active=false;}
bool ble_scan_active(){return sim_ble_active;}
int ble_scan_consumer_count(){return sim_ble_active?1:0;}
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
