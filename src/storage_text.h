#pragma once
#include "storage_manager.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

// Builds a small immutable text record without opening an SD file. The
// bounded builder rejects overflow instead of silently persisting a fragment.
class StorageText {
    static constexpr size_t capacity=2048;
    char* data_=static_cast<char*>(storage_allocate(capacity));
    size_t size_=0;
    bool valid_=true;
public:
    StorageText()=default;
    ~StorageText(){free(data_);}
    StorageText(const StorageText&)=delete;
    StorageText& operator=(const StorageText&)=delete;
    void printf(const char* format,...){
        if(!data_||!valid_)return;
        va_list args;va_start(args,format);
        int n=vsnprintf(data_+size_,capacity-size_,format,args);va_end(args);
        if(n<0 || size_t(n)>=capacity-size_){valid_=false;return;}
        size_+=n;
    }
    void print(const char* s){printf("%s",s);}
    void print(char c){printf("%c",c);}
    void println(const char* s){printf("%s\n",s);}
    bool commit(const char* path,StorageMode mode){
        char* p=data_;data_=nullptr;
        if(!valid_){free(p);p=nullptr;}
        return storage_submit_owned_write(path,p,size_,mode);
    }
};
