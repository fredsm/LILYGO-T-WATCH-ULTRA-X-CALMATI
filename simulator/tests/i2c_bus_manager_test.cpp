#include "../../src/i2c_bus_manager.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) std::abort(); } while(0)
using namespace std::chrono;

int main(){
    i2c_bus_init();
    CHECK(i2c_client(0x34)==I2cClient::Pmu && i2c_client(0x20)==I2cClient::Expander);
    CHECK(i2c_client(0x29)==I2cClient::Motion && i2c_client(0x1a)==I2cClient::Touch);
    CHECK(i2c_bus_timeout(200)==200);
    CHECK(i2c_bus_acquire(0x20));i2c_bus_release(I2cResult::Nack);
    CHECK(i2c_bus_ready(0x20)); // boot probing must not throttle
    i2c_bus_runtime(true);
    CHECK(i2c_bus_timeout(200)==50 && i2c_bus_timeout(25)==25);
    CHECK(i2c_bus_acquire(0x34));CHECK(!i2c_bus_ready(0x51));
    CHECK(i2c_bus_stats().active && i2c_bus_stats().address==0x34);
    std::atomic<bool> finished{false};
    std::thread waiter([&]{
        CHECK(!i2c_bus_acquire(0x51));finished=true;
    });
    waiter.join();
    CHECK(finished && i2c_bus_stats().admission_timeouts==1);
    i2c_bus_release(I2cResult::Timeout);
    auto s=i2c_bus_stats();
    CHECK(s.cooling && s.recovering && !s.active);
    CHECK(!i2c_bus_ready(0x51));CHECK(!i2c_bus_acquire(0x51));
    std::this_thread::sleep_for(milliseconds(120));
    CHECK(i2c_bus_ready(0x51));
    CHECK(i2c_bus_acquire(0x51));i2c_bus_release(I2cResult::Ok);
    s=i2c_bus_stats();
    CHECK(!s.recovering && !s.cooling && s.recoveries==1);
    CHECK(s.clients[static_cast<unsigned>(I2cClient::Pmu)].timeouts==1);
    CHECK(i2c_bus_acquire(0x28));i2c_bus_release(I2cResult::Nack);
    CHECK(!i2c_bus_ready(0x29));CHECK(i2c_bus_ready(0x34)); // NACK is local, not physical bus timeout
    CHECK(i2c_bus_acquire(0x34));i2c_bus_release(I2cResult::Ok);
    CHECK(i2c_bus_acquire(0x34));i2c_bus_release(I2cResult::Timeout);
    i2c_bus_runtime(false); // power lifecycle bypasses cooldown
    CHECK(i2c_bus_ready(0x34) && i2c_bus_timeout(200)==200);
    CHECK(i2c_bus_acquire(0x34));i2c_bus_release(I2cResult::Ok);
    i2c_bus_runtime(true);
    std::atomic<int> inside{0};
    auto run=[&](uint16_t address){
        for(int i=0;i<500;++i){
            CHECK(i2c_bus_acquire(address));
            CHECK(inside.fetch_add(1)==0);
            CHECK(inside.fetch_sub(1)==1);
            i2c_bus_release(I2cResult::Ok);
        }
    };
    std::thread a(run,0x34), b(run,0x51);
    a.join();b.join();CHECK(!i2c_bus_stats().active);
}
