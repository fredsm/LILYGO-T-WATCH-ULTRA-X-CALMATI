#pragma once
#include <deque>
#include <vector>
#include <cstring>
constexpr int pdTRUE=1,pdFALSE=0;
struct SimQueue {size_t capacity,item_size;std::deque<std::vector<unsigned char>> items;};
using QueueHandle_t=SimQueue*;
inline QueueHandle_t xQueueCreate(unsigned n,unsigned size){return new SimQueue{n,size,{}};}
inline int xQueueSend(QueueHandle_t q,const void* item,unsigned){if(!q||q->items.size()>=q->capacity)return pdFALSE;const auto* b=(const unsigned char*)item;q->items.emplace_back(b,b+q->item_size);return pdTRUE;}
inline int xQueueReceive(QueueHandle_t q,void* out,unsigned){if(!q||q->items.empty())return pdFALSE;std::memcpy(out,q->items.front().data(),q->item_size);q->items.pop_front();return pdTRUE;}
inline void vQueueDelete(QueueHandle_t q){delete q;}
inline void xQueueReset(QueueHandle_t q){if(q)q->items.clear();}
