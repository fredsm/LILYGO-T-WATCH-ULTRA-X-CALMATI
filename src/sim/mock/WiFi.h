#pragma once
#include "Arduino.h"
enum wifi_mode_t {WIFI_MODE_NULL,WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_APSTA};
constexpr auto WIFI_OFF=WIFI_MODE_NULL; constexpr auto WIFI_STA=WIFI_MODE_STA;
enum {WL_CONNECTED=3,WL_DISCONNECTED=6};
struct IPAddress {unsigned operator[](int i)const{return i==0?192:i==1?0:i==2?2:10;}};
struct SimWiFi {wifi_mode_t current=WIFI_MODE_NULL; void mode(wifi_mode_t v){current=v;} void disconnect(bool=false,bool=false){} int status()const{return WL_DISCONNECTED;} String SSID()const{return "SIM";} IPAddress localIP()const{return {};} int RSSI()const{return -55;} int softAPgetStationNum()const{return 0;}};
inline SimWiFi WiFi;
