#pragma once
#include <cstdint>
enum {FILE_READ,FILE_WRITE};
struct File { explicit operator bool() const {return false;} int available(){return 0;} int read(){return -1;} void close(){} template<class... T> int printf(const char*,T...){return 0;} };
struct SimSD { bool exists(const char*){return false;} bool mkdir(const char*){return true;} File open(const char*,int){return {};} 
 uint64_t totalBytes() const { return 32000000000ULL; }
 uint64_t usedBytes() const { return 4000000ULL; }
};
inline SimSD SD;
