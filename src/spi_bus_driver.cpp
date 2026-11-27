#ifndef TWATCH_SIMULATOR
#include <SPI.h>
#include "spi_bus_manager.h"

// Linker wraps the Arduino 2.x out-of-line transaction methods, including
// framework SD/MSC. Tagged vendor hooks use the same underlying methods.
// Non-global SPI objects and the IDF QSPI display never take this mutex.
extern "C" void __real__ZN8SPIClass16beginTransactionE11SPISettings(SPIClass*, SPISettings);
extern "C" void __real__ZN8SPIClass14endTransactionEv(SPIClass*);

extern "C" void twatch_spi_begin_tagged(SPIClass* spi, SPISettings settings, uint8_t client) {
    if(spi==&SPI) spi_bus_acquire(static_cast<SpiClient>(client));
    __real__ZN8SPIClass16beginTransactionE11SPISettings(spi, settings);
}
extern "C" void twatch_spi_end_tagged(SPIClass* spi) {
    __real__ZN8SPIClass14endTransactionEv(spi);
    if(spi==&SPI) spi_bus_release();
}
extern "C" void __wrap__ZN8SPIClass16beginTransactionE11SPISettings(SPIClass* spi, SPISettings settings) {
    twatch_spi_begin_tagged(spi, settings, static_cast<uint8_t>(SpiClient::Sd));
}
extern "C" void __wrap__ZN8SPIClass14endTransactionEv(SPIClass* spi) {
    twatch_spi_end_tagged(spi);
}
#endif
