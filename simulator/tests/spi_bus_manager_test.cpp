#include "../../src/spi_bus_manager.h"
#include <atomic>
#include <chrono>
#include <thread>
#include <cstdlib>
#define CHECK(x) do { if(!(x)) std::abort(); } while(0)

int main() {
    spi_bus_init();
    CHECK(spi_bus_ui_ready(SpiClient::LoRa));
    spi_bus_acquire(SpiClient::Sd);
    CHECK(!spi_bus_ui_ready(SpiClient::LoRa));
    CHECK(!spi_bus_ui_ready(SpiClient::Nfc));
    std::atomic<bool> entered{false};
    std::thread radio([&] {
        spi_bus_acquire(SpiClient::LoRa);
        entered=true;
        auto s=spi_bus_stats();
        CHECK(s.active && s.owner==SpiClient::LoRa);
        spi_bus_release();
    });
    while(!spi_bus_stats().waiters) std::this_thread::yield();
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
    CHECK(!entered);
    CHECK(spi_bus_stats().owner==SpiClient::Sd);
    spi_bus_release();
    radio.join();
    CHECK(entered && spi_bus_ui_ready(SpiClient::Nfc));
    spi_bus_acquire(SpiClient::Nfc);
    spi_bus_release();
    auto s=spi_bus_stats();
    CHECK(!s.active && !s.waiters);
    CHECK(s.clients[0].transactions==1 && s.clients[0].max_hold_us>=1000);
    CHECK(s.clients[1].transactions==1 && s.clients[1].contended==1);
    CHECK(s.clients[1].max_wait_us>=1000 && s.clients[1].deferred==1);
    CHECK(s.clients[2].transactions==1 && s.clients[2].deferred==1);
    // Exercise actual concurrent acquisitions; check no critical sections overlap.
    std::atomic<int> inside{0};
    auto work=[&](SpiClient c) {
        for(int i=0;i<1000;++i) {
            spi_bus_acquire(c);
            CHECK(inside.fetch_add(1)==0);
            CHECK(inside.fetch_sub(1)==1);
            spi_bus_release();
        }
    };
    std::thread a(work,SpiClient::Sd),b(work,SpiClient::LoRa),c(work,SpiClient::Nfc);
    a.join();b.join();c.join();
    s=spi_bus_stats();
    CHECK(!s.active && !s.waiters);
    for(auto& client:s.clients) CHECK(client.transactions==1001);
}
