#pragma once
#include <stdint.h>

// Global Arduino SPI only (SD/raw MSC, LoRa, ST25 NFC). Display QSPI is separate.
enum class SpiClient : uint8_t { Sd, LoRa, Nfc, Count };
struct SpiClientStats {
    uint32_t transactions=0, contended=0, deferred=0;
    uint32_t max_wait_us=0, max_hold_us=0;
};
struct SpiBusStats {
    SpiClientStats clients[static_cast<unsigned>(SpiClient::Count)]={};
    bool active=false, lifecycle=false;
    SpiClient owner=SpiClient::Sd;
    uint32_t active_us=0, waiters=0;
};
void spi_bus_init();
// Paired in task context, never from an ISR or nested inside another transaction.
void spi_bus_acquire(SpiClient client);
void spi_bus_release();
// Nonblocking advisory check for periodic UI jobs; does not reserve the bus.
bool spi_bus_ui_ready(SpiClient client);
// UI-only exclusive lifecycle guard: never wait for an active transaction.
bool spi_bus_lifecycle_begin();
void spi_bus_lifecycle_end();
SpiBusStats spi_bus_stats();
const char* spi_bus_client_name(SpiClient client);
