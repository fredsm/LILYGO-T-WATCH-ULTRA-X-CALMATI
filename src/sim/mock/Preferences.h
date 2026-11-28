#pragma once
#include "Arduino.h"
#include <map>
#include <string>
#include <cstdlib>
// Session-only NVS for firmware settings. Never writes user data to disk.
class Preferences {
    std::string ns;
    bool readonly=true;
    inline static std::map<std::string,std::string> values;
public:
    bool begin(const char* name,bool ro=false){ns=name;readonly=ro;return true;}
    void end(){ns.clear();readonly=true;}
    String getString(const char* key,const char* fallback="")const{auto it=values.find(ns+"/"+key);return it==values.end()?String(fallback):String(it->second);}
    bool remove(const char* key){return !readonly&&values.erase(ns+"/"+key)>0;}
    bool getBool(const char* key,bool fallback=false)const{
        auto it=values.find(ns+"/"+key);return it==values.end()?fallback:it->second=="1";
    }
    uint32_t getUInt(const char* key,uint32_t fallback=0)const{
        auto it=values.find(ns+"/"+key);
        if(it==values.end())return fallback;
        char* end=nullptr;
        unsigned long long v=std::strtoull(it->second.c_str(),&end,10);
        return end!=it->second.c_str() && *end=='\0' && v<=UINT32_MAX ? static_cast<uint32_t>(v) : fallback;
    }
    size_t putUInt(const char* key,uint32_t value){
        if(readonly||ns.empty())return 0;
        values[ns+"/"+key]=std::to_string(value);return sizeof(uint32_t);
    }
    size_t putBool(const char* key,bool value){return putString(key,value?"1":"0");}
    size_t putString(const char* key,const char* value){
        if(readonly||ns.empty())return 0;values[ns+"/"+key]=value;return std::strlen(value);
    }
};
