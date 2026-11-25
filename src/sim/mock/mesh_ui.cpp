#include "../../meshtastic.h"
#include "../../esp_now_link.h"
#include "../../car_finder.h"
#include <lvgl.h>
#include <cstdio>
#include <cmath>
#include <vector>
#include <cstring>
void tools_screen_show();
void meshtastic_mark_read(){}
void clock_screen_set_gps_active(bool){}
void clock_screen_set_sat_count(uint32_t){}
void clock_screen_set_utc_offset(int){}
bool clock_screen_manual_time_active(){return true;}
void timezone_note_detected(int){}
void main_loop_request_lvgl_priority(int){}



namespace { bool saved=false; double lat=0,lon=0; char lname[40]="Sim Watch",sname[5]="SIM"; bool announce=true; uint32_t interval=600000; int channel=0; MeshChannel channels[4]={{"LongFast",{},1,0,true},{"Demo",{},0,0,false},{},{}}; std::vector<MeshOutgoing> outgoing; std::vector<MeshMessage> messages; MeshTraceroute trace{}; }
bool car_finder_save(double a,double b){lat=a;lon=b;saved=true;return true;}
bool car_finder_clear(){saved=false;return true;}
bool car_finder_has(){return saved;}
bool car_finder_get(double* a,double* b){if(!saved)return false;*a=lat;*b=lon;return true;}
double geo_distance_m(double a,double b,double c,double d){return std::hypot(c-a,d-b)*111000;}
double geo_bearing_deg(double a,double b,double c,double d){return std::fmod(std::atan2(d-b,c-a)*180/3.141592653589793+360,360);}
int meshtastic_get_node_count(){return 1;}
const MeshNode* meshtastic_get_node(int i){static MeshNode n=[](){MeshNode v{};v.node_id=123;std::strcpy(v.id,"!0000007b");std::strcpy(v.long_name,"Simulated node");std::strcpy(v.short_name,"SIM1");std::strcpy(v.time_str,"12:00");v.has_telemetry=true;v.has_battery_level=true;v.battery_level=80;return v;}();return i==0?&n:nullptr;}
const char* meshtastic_role_name(uint32_t){return "Client";}
bool meshtastic_send_traceroute(uint32_t id){trace={};trace.target_node=id;trace.has_response=true;trace.response_ms=lv_tick_get();return true;}
const MeshTraceroute* meshtastic_get_last_traceroute(){return &trace;}
bool meshtastic_request_position(uint32_t){return false;}
bool meshtastic_send_text_to(const char* text,uint32_t id){MeshOutgoing o{};o.packet_id=lv_tick_get();o.sent_ms=lv_tick_get();o.dest_node=id;o.state=MeshOutgoing::DELIVERED;std::snprintf(o.text,sizeof(o.text),"%s",text);outgoing.insert(outgoing.begin(),o);if(outgoing.size()>6)outgoing.pop_back();return true;}
int meshtastic_get_outgoing_count(){return int(outgoing.size());}
const MeshOutgoing* meshtastic_get_outgoing(int i){return i>=0&&i<int(outgoing.size())?&outgoing[i]:nullptr;}
const char* meshtastic_routing_error_name(uint8_t){return "Simulated error";}
void meshtastic_set_long_name(const char* v){std::snprintf(lname,sizeof(lname),"%s",v);}
void meshtastic_set_short_name(const char* v){std::snprintf(sname,sizeof(sname),"%s",v);}
const char* meshtastic_get_long_name(){return lname;}
const char* meshtastic_get_short_name(){return sname;}
void meshtastic_set_announce(bool v,uint32_t ms){announce=v;interval=ms;}
bool meshtastic_announce_enabled(){return announce;}
uint32_t meshtastic_announce_interval_ms(){return interval;}
const MeshChannel* meshtastic_get_channel(int i){return i>=0&&i<4?&channels[i]:nullptr;}
void meshtastic_set_channel_enabled(int i,bool v){if(i>=0&&i<4)channels[i].enabled=v;}
int meshtastic_get_active_channel(){return channel;}
void meshtastic_set_active_channel(int i){if(i>=0&&i<4)channel=i;}
int meshtastic_get_count(){return int(messages.size());}
const MeshMessage* meshtastic_get_message(int i){return i>=0&&i<int(messages.size())?&messages[i]:nullptr;}
void meshtastic_clear_messages(){messages.clear();}
bool meshtastic_delete_message(int i){if(i<0||i>=int(messages.size()))return false;messages.erase(messages.begin()+i);return true;}
uint32_t meshtastic_get_node_id(){return 1;}
