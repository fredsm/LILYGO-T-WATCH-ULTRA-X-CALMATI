#pragma once
#include "WiFiClient.h"
// No transport: return a fixed voice-assistant response for any simulated URL.
struct HTTPClient {
 void setConnectTimeout(int){} void setTimeout(int){} void setReuse(bool){}
 bool begin(WiFiClient&,const char*){return true;}
 void addHeader(const char*,const char*){}
 int POST(const uint8_t*,size_t){return 200;}
 String getString(){return "SIM voice request\n---\nSimulated assistant response.";}
 void end(){}
};
