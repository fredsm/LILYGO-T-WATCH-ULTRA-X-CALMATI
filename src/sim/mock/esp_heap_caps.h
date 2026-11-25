#pragma once
#include <cstdlib>
#define MALLOC_CAP_SPIRAM 1
inline void* heap_caps_malloc(size_t n,int){return std::malloc(n);}
inline void heap_caps_free(void* p){std::free(p);}

#define MALLOC_CAP_8BIT 2
inline void* heap_caps_calloc(size_t n,size_t s,int){return std::calloc(n,s);}
