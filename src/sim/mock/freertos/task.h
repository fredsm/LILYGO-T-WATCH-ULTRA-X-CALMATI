#pragma once
using TaskHandle_t=void*;
inline int xTaskCreatePinnedToCore(void(*)(void*),const char*,unsigned,void*,int,TaskHandle_t*,int){return 0;}
inline void vTaskDelay(unsigned){}
inline void vTaskDelete(TaskHandle_t){}
