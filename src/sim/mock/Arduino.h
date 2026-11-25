#pragma once
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cctype>
#include <string>
#include <type_traits>
#include <lvgl.h>
#include "LilyGoLib.h"
class String {
 std::string s;
public:
 String()=default; String(const char* p):s(p?p:""){} String(std::string p):s(p){}
 template<class T,typename std::enable_if<std::is_arithmetic<T>::value,int>::type=0> String(T v):s(std::to_string(v)){}
 const char* c_str() const{return s.c_str();} unsigned length()const{return s.size();}
 char operator[](unsigned i)const{return i<s.size()?s[i]:0;}
 int indexOf(const char* p,unsigned from=0)const{auto i=s.find(p,from);return i==std::string::npos?-1:int(i);}
 String substring(unsigned a)const{return substring(a,s.size());}
 int indexOf(char c)const{auto i=s.find(c);return i==std::string::npos?-1:int(i);}
 void trim(){auto a=s.find_first_not_of(" \t\r\n");if(a==std::string::npos){s.clear();return;}s=s.substr(a,s.find_last_not_of(" \t\r\n")-a+1);}
 long toInt()const{return std::strtol(s.c_str(),nullptr,10);}
 friend bool operator==(const String& a,const String& b){return a.s==b.s;}
 String substring(unsigned a,unsigned b)const{return a<s.size()&&b>=a?String(s.substr(a,b-a)):String();}
 String& operator+=(char c){s+=c;return *this;}
 friend String operator+(const String& a,const String& b){return String(a.s+b.s);}
};
inline bool isDigit(char c){return std::isdigit(static_cast<unsigned char>(c));}

inline void delay(unsigned long){}

constexpr double DEG_TO_RAD=0.017453292519943295;
constexpr double RAD_TO_DEG=57.29577951308232;

#ifndef __GNUC__
#define __attribute__(x)
#endif
inline size_t strlcpy(char* out,const char* in,size_t n){size_t len=std::strlen(in);if(n){size_t copy=len<n-1?len:n-1;std::memcpy(out,in,copy);out[copy]=0;}return len;}

constexpr double PI=3.14159265358979323846;
