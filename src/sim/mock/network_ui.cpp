#include "../../esp_now_link.h"
#include "../../calmati_remote.h"
#include "../../meshtastic.h"
#include <cstring>
namespace {bool active=false,mesh=false,connected=false; EnowPeer peer={{2,0,0,0,0,1},123,"SIM peer",0,false};}
void clock_screen_set_lora_active(bool){}
void meshtastic_set_active(bool v){mesh=v;}
float meshtastic_get_rssi(){return -70;}
bool meshtastic_get_cad(){return false;}
int meshtastic_get_rx_count(){return 0;}
bool esp_now_link_set_active(bool v){active=v;return true;}
bool esp_now_link_is_active(){return active;}
uint32_t esp_now_link_self_id(){return 1;}
const char* esp_now_link_self_name(){return "SIM watch";}
uint8_t esp_now_link_channel(){return 6;}
int esp_now_link_peer_count(){return active?1:0;}
const EnowPeer* esp_now_link_peer(int i){return active&&i==0?&peer:nullptr;}
const EnowPeer* esp_now_link_peer_by_id(uint32_t id){return active&&id==peer.id?&peer:nullptr;}
const EnowMessage* esp_now_link_msg(int){return nullptr;}
void esp_now_link_mark_read(){}
bool esp_now_link_send_text_to(const char*,uint32_t){return false;}
bool calmati_connect(uint32_t){connected=true;return true;}
bool calmati_is_connected(){return connected;}
bool calmati_send(const char*,String& out){out="Disabled in simulator";return false;}
bool calmati_status(String& out){out="{}";return true;}
bool calmati_aps(String& out){out="[]";return true;}
bool calmati_get(const char*,String& out){out="[]";return true;}
const char* calmati_ap_ssid(){return "SIM-CALMATI";}
