"""Tag pinned SPI drivers at the transaction boundary; safe on repeat builds."""
from pathlib import Path

DECL = ('extern "C" void twatch_spi_begin_tagged(SPIClass*, SPISettings, uint8_t);\n'
        'extern "C" void twatch_spi_end_tagged(SPIClass*);\n')

def patch_source(path, anchor, replacements):
    src = path.read_text(encoding="utf-8")
    if DECL not in src:
        if src.count(anchor) != 1:
            raise RuntimeError(f"Unexpected SPI include in {path}")
        src = src.replace(anchor, anchor + "\n" + DECL)
    for old, new, count in replacements:
        if src.count(old) == count and src.count(new) == 0:
            src = src.replace(old, new)
        elif src.count(old) != 0 or src.count(new) != count:
            raise RuntimeError(f"Unexpected SPI transactions in {path}: {old}")
    path.write_text(src, encoding="utf-8")

def patch_clients(root):
    radio = root / "RadioLib/src/hal/Arduino/ArduinoHal.cpp"
    patch_source(radio, '#include "ArduinoHal.h"', [
        ("spi->beginTransaction(spiSettings);", "twatch_spi_begin_tagged(spi, spiSettings, 1);", 1),
        ("spi->endTransaction();", "twatch_spi_end_tagged(spi);", 1),
    ])
    candidates = list(root.glob("*/src/st25r3916_com.cpp"))
    if len(candidates) != 1:
        raise RuntimeError(f"Expected one ST25R3916 driver, found {candidates}")
    patch_source(candidates[0], '#include "rfal_rfst25r3916.h"', [
        ("dev_spi->beginTransaction(SPISettings(spi_speed, MSBFIRST, SPI_MODE1));",
         "twatch_spi_begin_tagged(dev_spi, SPISettings(spi_speed, MSBFIRST, SPI_MODE1), 2);", 11),
        ("dev_spi->endTransaction();", "twatch_spi_end_tagged(dev_spi);", 11),
    ])

try:
    Import("env")
except NameError:
    pass
else:
    patch_clients(Path(env.subst("$PROJECT_LIBDEPS_DIR")) / env.subst("$PIOENV"))
