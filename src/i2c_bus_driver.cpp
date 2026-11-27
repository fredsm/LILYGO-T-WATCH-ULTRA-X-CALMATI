#ifndef TWATCH_SIMULATOR
#include <esp32-hal-i2c.h>
#include "i2c_bus_manager.h"

// Arduino 2.x Wire combines repeated-start register reads in one HAL call.
// The native Wire mutex protects its buffer/transaction construction; the
// manager sits below it, never acquires Wire itself, and never changes pins.
extern "C" esp_err_t __real_i2cWrite(uint8_t,uint16_t,const uint8_t*,size_t,uint32_t);
extern "C" esp_err_t __real_i2cRead(uint8_t,uint16_t,uint8_t*,size_t,uint32_t,size_t*);
extern "C" esp_err_t __real_i2cWriteReadNonStop(uint8_t,uint16_t,const uint8_t*,size_t,uint8_t*,size_t,uint32_t,size_t*);
namespace {
I2cResult classify(esp_err_t e){
    if(e==ESP_OK)return I2cResult::Ok;
    if(e==ESP_ERR_TIMEOUT)return I2cResult::Timeout;
    if(e==ESP_FAIL)return I2cResult::Nack; // Arduino maps ESP_FAIL to NACK
    return I2cResult::Error;
}
}
extern "C" esp_err_t __wrap_i2cWrite(uint8_t bus,uint16_t addr,const uint8_t* data,size_t size,uint32_t ms){
    if(bus!=0)return __real_i2cWrite(bus,addr,data,size,ms);
    if(!i2c_bus_acquire(addr))return ESP_ERR_TIMEOUT;
    auto r=__real_i2cWrite(bus,addr,data,size,i2c_bus_timeout(ms));
    i2c_bus_release(classify(r));return r;
}
extern "C" esp_err_t __wrap_i2cRead(uint8_t bus,uint16_t addr,uint8_t* data,size_t size,uint32_t ms,size_t* count){
    if(bus!=0)return __real_i2cRead(bus,addr,data,size,ms,count);
    if(!i2c_bus_acquire(addr)){*count=0;return ESP_ERR_TIMEOUT;}
    auto r=__real_i2cRead(bus,addr,data,size,i2c_bus_timeout(ms),count);
    if(r!=ESP_OK)*count=0;
    i2c_bus_release(classify(r));return r;
}
extern "C" esp_err_t __wrap_i2cWriteReadNonStop(uint8_t bus,uint16_t addr,const uint8_t* w,size_t wn,uint8_t* r,size_t rn,uint32_t ms,size_t* count){
    if(bus!=0)return __real_i2cWriteReadNonStop(bus,addr,w,wn,r,rn,ms,count);
    if(!i2c_bus_acquire(addr)){*count=0;return ESP_ERR_TIMEOUT;}
    auto e=__real_i2cWriteReadNonStop(bus,addr,w,wn,r,rn,i2c_bus_timeout(ms),count);
    if(e!=ESP_OK)*count=0;
    i2c_bus_release(classify(e));return e;
}
#endif
