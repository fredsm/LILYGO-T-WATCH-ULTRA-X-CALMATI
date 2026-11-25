#pragma once
#include "Arduino.h"
enum wifi_mode_t {WIFI_MODE_NULL,WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_APSTA};
constexpr auto WIFI_OFF=WIFI_MODE_NULL; constexpr auto WIFI_STA=WIFI_MODE_STA;
enum {WL_CONNECTED=3,WL_DISCONNECTED=6};
struct IPAddress {uint32_t raw=0x0a0200c0; IPAddress()=default; explicit IPAddress(uint32_t v):raw(v){} explicit operator uint32_t()const{return raw;}unsigned operator[](int i)const{return (raw>>(8*i))&255;}};
struct SimWiFi {wifi_mode_t current=WIFI_MODE_NULL; void mode(wifi_mode_t v){current=v;} bool connected=false; String ssid="SIM"; void disconnect(bool=false,bool=false){connected=false;} int status()const{return connected?WL_CONNECTED:WL_DISCONNECTED;} void begin(const char* s,const char* =nullptr){ssid=s;connected=true;} String SSID(int=-1)const{return ssid;} void scanDelete(){} int scanNetworks(bool){return 1;} int scanComplete(){return 1;} int channel(int=0)const{return 6;} int encryptionType(int)const{return 0;} IPAddress localIP()const{return {};} int RSSI(int=-1)const{return -55;} int softAPgetStationNum()const{return 0;}};
inline SimWiFi WiFi;

constexpr int WIFI_SCAN_FAILED=-2,WIFI_AUTH_OPEN=0;
