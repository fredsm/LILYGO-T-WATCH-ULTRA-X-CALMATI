#pragma once
#include "SD.h"

// Independent virtual flash filesystem, retained across simulated deep reboot.
inline bool sim_ffat_available=true, sim_ffat_mounted=false;
inline bool sim_ffat_fail_write=false, sim_ffat_fail_publish=false;
inline std::map<std::string,std::shared_ptr<std::string>> sim_ffat_files;
inline std::set<std::string> sim_ffat_dirs;
struct SimFFat {
 bool begin(bool formatOnFail=false,const char* ="/fs"){
    if(!sim_ffat_available&&formatOnFail){sim_ffat_files.clear();sim_ffat_dirs.clear();sim_ffat_available=true;}
    return sim_ffat_mounted=sim_ffat_available;
 }
 bool exists(const char* p){return sim_ffat_mounted&&(sim_ffat_files.count(p)||sim_ffat_dirs.count(p));}
 bool mkdir(const char* p){if(!sim_ffat_mounted)return false;sim_ffat_dirs.insert(p);return true;}
 File open(const char* p,int mode=FILE_READ){
    if(!sim_ffat_mounted)return {};
    auto it=sim_ffat_files.find(p);
    if(mode==FILE_READ)return it==sim_ffat_files.end()?File():File(it->second,false,false,&sim_ffat_mounted);
    if(sim_ffat_fail_write){sim_ffat_fail_write=false;return {};}
    auto& b=sim_ffat_files[p];if(!b)b=std::make_shared<std::string>();
    if(mode==FILE_WRITE)b->clear();return File(b,true,mode==FILE_APPEND,&sim_ffat_mounted);
 }
 bool remove(const char* p){return sim_ffat_mounted&&sim_ffat_files.erase(p)>0;}
 bool rename(const char* from,const char* to){
    if(sim_ffat_fail_publish&&std::string(from).find(".tmp")!=std::string::npos){sim_ffat_fail_publish=false;return false;}
    if(!sim_ffat_mounted||sim_ffat_files.count(to))return false;
    auto it=sim_ffat_files.find(from);if(it==sim_ffat_files.end())return false;
    sim_ffat_files[to]=it->second;sim_ffat_files.erase(it);return true;
 }
};
inline SimFFat FFat;
