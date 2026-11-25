#include "../../alarm.h"
namespace { int hour=7,minute=0,snooze=5; bool enabled=false,configured=false,vibrate=true,audio=true,ringing=false; uint8_t volume=100; }
void alarm_init(){}
void alarm_tick(){}
int alarm_get_hour(){return hour;}
int alarm_get_minute(){return minute;}
bool alarm_is_enabled(){return enabled;}
void alarm_set(int h,int m,bool e){hour=h;minute=m;enabled=e;configured=true;}
bool alarm_is_ringing(){return ringing;}
void alarm_dismiss(){ringing=false;}
void alarm_snooze(int){ringing=false;}
void alarm_get_next_fire_time(int* h,int* m){*h=hour;*m=minute;}
bool alarm_is_configured(){return configured;}
bool alarm_get_vibrate(){return vibrate;}
void alarm_set_vibrate(bool v){vibrate=v;}
bool alarm_get_audio(){return audio;}
void alarm_set_audio(bool v){audio=v;}
uint8_t alarm_get_volume(){return volume;}
void alarm_set_volume(uint8_t v){volume=v;}
int alarm_get_snooze_minutes(){return snooze;}
void alarm_set_snooze_minutes(int v){snooze=v;}
void alarm_play_chime_loop(uint8_t){}
void alarm_stop_chime_loop(){}
