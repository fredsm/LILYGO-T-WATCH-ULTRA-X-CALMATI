#pragma once
#include "Arduino.h"
#include <map>
#include <string>
// Session-only NVS for the original onboarding UI. Never writes user data to disk.
class Preferences {
    std::string ns;
    bool readonly=true;
    inline static std::map<std::string,std::string> values;
public:
    bool begin(const char* name,bool ro=false){ns=name;readonly=ro;return true;}
    void end(){ns.clear();readonly=true;}
    bool getBool(const char* key,bool fallback=false)const{
        auto it=values.find(ns+"/"+key);return it==values.end()?fallback:it->second=="1";
    }
    size_t putBool(const char* key,bool value){return putString(key,value?"1":"0");}
    size_t putString(const char* key,const char* value){
        if(readonly||ns.empty())return 0;values[ns+"/"+key]=value;return std::strlen(value);
    }
};
