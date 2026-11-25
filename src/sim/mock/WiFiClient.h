#pragma once
#include "WiFi.h"
struct WiFiClient {void setTimeout(int){} bool connect(const char*,int,int){return true;} bool connect(IPAddress,int,int){return false;} void stop(){} size_t write(const uint8_t*,size_t){return 0;} void flush(){}};
