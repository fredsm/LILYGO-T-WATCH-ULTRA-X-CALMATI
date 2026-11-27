#pragma once
#include "Arduino.h"
#include <map>
#include <set>
#include <memory>
#include <vector>
#include <algorithm>
// Volatile virtual SD: every path is an in-memory key, never a host file.
enum {FILE_READ,FILE_WRITE,FILE_APPEND};
inline std::map<std::string,std::shared_ptr<std::string>> sim_sd_files;
inline std::set<std::string> sim_sd_dirs;
class File {
 std::shared_ptr<std::string> bytes;size_t pos=0;bool writable=false;
 const bool* ready=&sim::hardware.sd_ready;
 bool media_ready()const{return *ready;}
public:
 File()=default;
 File(std::shared_ptr<std::string> b,bool w,bool append=false,const bool* media=&sim::hardware.sd_ready):bytes(b),pos(append?b->size():0),writable(w),ready(media){}
 explicit operator bool()const{return media_ready()&&bool(bytes);}
 size_t size()const{return media_ready()&&bytes?bytes->size():0;}
 int available()const{return media_ready()&&bytes&&pos<bytes->size()?int(bytes->size()-pos):0;}
 int read(){return available()?uint8_t((*bytes)[pos++]):-1;}
 size_t read(uint8_t* out,size_t n){n=std::min(n,size_t(available()));if(n)std::memcpy(out,bytes->data()+pos,n);pos+=n;return n;}
 size_t readBytes(char* out,size_t n){return read((uint8_t*)out,n);}
 String readStringUntil(char end){std::string out;int c;while((c=read())>=0&&c!=end)out+=char(c);return out;}
 bool seek(size_t p){if(!media_ready()||!bytes||p>bytes->size())return false;pos=p;return true;}
 size_t position()const{return pos;}
 size_t write(const uint8_t* data,size_t n){if(!media_ready()||!bytes||!writable)return 0;if(pos+n>bytes->size())bytes->resize(pos+n);std::memcpy(bytes->data()+pos,data,n);pos+=n;return n;}
 size_t print(const char* s){return write((const uint8_t*)s,std::strlen(s));}
 size_t println(const char* s){size_t n=print(s);return n+print("\n");}
 template<class... T> int printf(const char* fmt,T... args){int n=std::snprintf(nullptr,0,fmt,args...);if(n<0)return 0;std::vector<char> b(n+1);std::snprintf(b.data(),b.size(),fmt,args...);return int(write((const uint8_t*)b.data(),n));}
 void close(){bytes.reset();} void flush(){}
};
struct SimSD {
 bool exists(const char* p){return sim::hardware.sd_ready&&(sim_sd_files.count(p)||sim_sd_dirs.count(p));}
 bool mkdir(const char* p){if(!sim::hardware.sd_ready)return false;sim_sd_dirs.insert(p);return true;}
 File open(const char* p,int mode=FILE_READ){
  if(!sim::hardware.sd_ready)return {};
  auto it=sim_sd_files.find(p);if(mode==FILE_READ)return it==sim_sd_files.end()?File():File(it->second,false);
  auto& b=sim_sd_files[p];if(!b)b=std::make_shared<std::string>();if(mode==FILE_WRITE)b->clear();return File(b,true,mode==FILE_APPEND);
 }
 bool remove(const char* p){return sim::hardware.sd_ready&&sim_sd_files.erase(p)>0;}
 bool rename(const char* from,const char* to){if(!sim::hardware.sd_ready||sim_sd_files.count(to))return false;auto it=sim_sd_files.find(from);if(it==sim_sd_files.end())return false;sim_sd_files[to]=it->second;sim_sd_files.erase(it);return true;}
 uint64_t totalBytes()const{return sim::hardware.sd_ready?32000000000ULL:0;}
 uint64_t usedBytes()const{if(!sim::hardware.sd_ready)return 0;uint64_t n=0;for(const auto& p:sim_sd_files)n+=p.second->size();return n;}
};
inline SimSD SD;
