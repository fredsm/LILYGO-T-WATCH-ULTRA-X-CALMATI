#pragma once
using TaskHandle_t=void*;
inline int xTaskCreatePinnedToCore(void(*fn)(void*),const char*,unsigned,void* arg,int,TaskHandle_t* handle,int){if(handle)*handle=reinterpret_cast<void*>(1);fn(arg);return 1;}
inline void vTaskDelay(unsigned){}
inline void vTaskDelete(TaskHandle_t){}
