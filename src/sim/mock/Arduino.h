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
 String substring(unsigned a,unsigned b)const{return a<s.size()&&b>=a?String(s.substr(a,b-a)):String();}
 String& operator+=(char c){s+=c;return *this;}
 friend String operator+(const String& a,const String& b){return String(a.s+b.s);}
};
inline bool isDigit(char c){return std::isdigit(static_cast<unsigned char>(c));}
