#pragma once
#include "Arduino.h"
enum wifi_mode_t {WIFI_MODE_NULL,WIFI_MODE_STA,WIFI_MODE_AP,WIFI_MODE_APSTA};
constexpr auto WIFI_OFF=WIFI_MODE_NULL; constexpr auto WIFI_STA=WIFI_MODE_STA;
enum {WL_CONNECTED=3,WL_DISCONNECTED=6};
struct IPAddress {uint32_t raw=0x0a0200c0; IPAddress()=default; explicit IPAddress(uint32_t v):raw(v){} explicit operator uint32_t()const{return raw;}unsigned operator[](int i)const{return (raw>>(8*i))&255;}};
using wifi_auth_mode_t=int;
constexpr int WIFI_AUTH_WPA3_PSK=6,WIFI_AUTH_WPA2_WPA3_PSK=7;
struct SimWiFi {uint8_t* BSSID(int=0){static uint8_t mac[]={2,0,0,0,0,1};return mac;} wifi_mode_t getMode() const{return current;} wifi_mode_t current=WIFI_MODE_NULL; void mode(wifi_mode_t v){current=v;} bool connected=false; String ssid="SIM"; void disconnect(bool=false,bool=false){connected=false;} int status()const{return connected?WL_CONNECTED:WL_DISCONNECTED;} void begin(const char* s,const char* =nullptr){ssid=s;connected=true;} String SSID(int=-1)const{return ssid;} void scanDelete(){} int scanNetworks(bool){return 1;} int scanComplete(){return 1;} int channel(int=0)const{return 6;} wifi_auth_mode_t encryptionType(int)const{return WIFI_AUTH_WPA3_PSK;} IPAddress localIP()const{return {};} int RSSI(int=-1)const{return -55;} int softAPgetStationNum()const{return 0;}};
inline SimWiFi WiFi;

constexpr int WIFI_SCAN_FAILED=-2,WIFI_AUTH_OPEN=0;
