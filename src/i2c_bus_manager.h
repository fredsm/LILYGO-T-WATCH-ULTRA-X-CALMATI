#pragma once
#include <stdint.h>

enum class I2cClient : uint8_t { Pmu, Touch, TouchHaptic, Rtc, Motion, Expander, Other, Count };
enum class I2cResult : uint8_t { Ok, Nack, Timeout, Error };
struct I2cClientStats {
    uint32_t transactions=0, nacks=0, timeouts=0, errors=0, deferred=0;
    uint32_t max_wait_us=0, max_transfer_us=0;
};
struct I2cBusStats {
    I2cClientStats clients[static_cast<unsigned>(I2cClient::Count)]={};
    bool active=false, cooling=false, recovering=false;
    uint16_t address=0;
    uint32_t active_us=0, failures=0, recoveries=0, admission_timeouts=0;
};
void i2c_bus_init();
// Boot and sleep retain upstream timing; runtime enables bounded transfers and cooldown.
void i2c_bus_runtime(bool enabled);
I2cClient i2c_client(uint16_t address);
const char* i2c_client_name(I2cClient client);
bool i2c_bus_ready(uint16_t address);
uint32_t i2c_bus_failures();
bool i2c_bus_acquire(uint16_t address);
void i2c_bus_release(I2cResult result);
uint32_t i2c_bus_timeout(uint32_t requested_ms);
I2cBusStats i2c_bus_stats();
