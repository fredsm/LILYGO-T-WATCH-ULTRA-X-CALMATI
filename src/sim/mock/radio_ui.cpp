#include "../../pager.h"
#include "../../tpms.h"
#include "../../aprs.h"
#include "../../adsb.h"
#include "../../mic_rec.h"
#include <lvgl.h>
#include "SD.h"
#include <cstdio>
#include <cstring>
namespace { bool p=false,scan=false,t=false,a=false,d=false,m=false; float freq=433.92f; PagerMode mode=PAGER_POCSAG_1200; TpmsFormat fmt=TPMS_FORMAT_FSK; bool f433=true; char call[16]="SIM-1"; uint32_t start=0,elapsed=0,seq=0; }

bool pager_start(float f,PagerMode v){freq=f;mode=v;p=true;scan=false;return true;}
void pager_stop(){p=scan=false;}
bool pager_is_running(){return p;}
float pager_get_freq(){return freq;}
PagerMode pager_get_mode(){return mode;}
void pager_set_mode(PagerMode v){mode=v;}
int16_t pager_last_error(){return 0;}
void pager_bg_tick(){}
int pager_get_msg_count(){return 0;}
const PagerMsg* pager_get_msg(int){return nullptr;}
void pager_clear_msgs(){}
bool pager_start_scanner(PagerMode v){pager_start(freq,v);scan=true;return true;}
bool pager_is_scanning_all(){return scan;}
int pager_scan_freq_count(){return 3;}
float pager_scan_freq_at(int){return freq;}
int pager_scan_current_idx(){return 0;}
bool pager_transmit_pocsag(uint32_t,PagerFunc,const char*,PagerMode,float){return false;}
bool pager_transmit_flex(uint32_t,const char*,float){return false;}
bool tpms_start(bool f,TpmsFormat v){t=true;f433=f;fmt=v;return true;}
void tpms_stop(){t=false;}
bool tpms_is_running(){return t;}
bool tpms_is_freq_433(){return f433;}
TpmsFormat tpms_get_format(){return fmt;}
void tpms_bg_tick(){}
const TpmsSensor* tpms_get_sensor(TpmsPos pos){static TpmsSensor s[4]{};if(t){auto& x=s[static_cast<int>(pos)];x={uint32_t(100+pos),230.0f,22,false,false,lv_tick_get(),true};}return &s[static_cast<int>(pos)];}
int tpms_get_total_count(){return t?4:0;}
int16_t tpms_last_error(){return 0;}
int tpms_get_isr_count(){return 0;}
int tpms_get_crc_count(){return 0;}
int tpms_get_sync_count(){return 0;}
int tpms_get_raw_sync_count(){return 0;}
int tpms_get_man_count(){return 0;}
int tpms_get_last_n_chips(){return 0;}
int tpms_get_max_n_chips(){return 0;}
uint8_t tpms_get_last_chip8(){return 0;}
float tpms_get_ook_threshold(){return -90;}
void aprs_init(){}
bool aprs_start(){a=true;return true;}
void aprs_stop(){a=false;}
bool aprs_is_running(){return a;}
int16_t aprs_last_error(){return 0;}
bool aprs_send_position(const char*){return false;}
bool aprs_tx_pending(){return false;}
bool aprs_tx_busy(){return false;}
void aprs_bg_tick(){}
int aprs_get_packet_count(){return 0;}
const AprsPacket* aprs_get_packet(int){return nullptr;}
void aprs_clear_packets(){}
const char* aprs_get_callsign(){return call;}
void aprs_set_callsign(const char* v){std::snprintf(call,sizeof(call),"%s",v);}
void adsb_start(){d=true;++seq;}
void adsb_stop(){d=false;++seq;}
bool adsb_is_running(){return d;}
int adsb_get(AdsbAircraft* out,int max){if(!d||max<1)return 0;*out={"SIM001","000001",0,0,12000,220,90,3,45,false};return 1;}
int adsb_count(){return d?1:0;}
AdsbStatus adsb_status(){return d?ADSB_OK:ADSB_IDLE;}
const char* adsb_status_text(){return d?"Simulated traffic":"Idle";}
uint32_t adsb_age_ms(){return 0;}
uint32_t adsb_seq(){return seq;}
bool adsb_center(float* lat,float* lon){*lat=0;*lon=0;return d;}
namespace { int range=10; }
int adsb_range_nm(){return range;}
void adsb_cycle_range(){range=range==40?10:range*2;++seq;}
static uint32_t mic_session=0;
void mic_rec_start(){if(!m){++mic_session;start=lv_tick_get();elapsed=0;m=true;}}
void mic_rec_stop(){if(m){elapsed=lv_tick_get()-start;m=false;auto f=SD.open("/Recordings/SIM.wav",FILE_WRITE);std::string wav(32044,0);std::memcpy(wav.data(),"RIFF",4);std::memcpy(wav.data()+8,"WAVE",4);f.write((const uint8_t*)wav.data(),wav.size());}}
void mic_rec_toggle(){if(m)mic_rec_stop();else mic_rec_start();}
bool mic_rec_is_recording(){return m;}
bool mic_rec_start_voice(){if(m)return false;mic_rec_start();return true;}
bool mic_rec_saved(){return !m && elapsed>0;}
uint32_t mic_rec_session(){return mic_session;}
bool mic_rec_copy_path(char* out,size_t n){if(!out || !n)return false;snprintf(out,n,"%s",mic_rec_last_path());return true;}
uint32_t mic_rec_elapsed_ms(){return m?lv_tick_get()-start:elapsed;}
size_t mic_rec_bytes(){return mic_rec_elapsed_ms()*32ULL;}
int mic_rec_level(){return m?35:0;}
const char* mic_rec_last_path(){return "/Recordings/SIM.wav";}
const char* mic_rec_status_text(){return m?"Simulated recording":"Idle";}
