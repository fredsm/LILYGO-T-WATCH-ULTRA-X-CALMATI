# Original firmware UI simulator

The native build reuses all **61 `src/*_screen.cpp` translation units**, plus the
original task manager. Screens are not reimplemented. The SDL launcher and CMake
test inventory both read `src/sim/screens.inc`.

All hardware boundaries resolve to `src/sim/mock`. Radio, network, BLE, USB/HID,
NFC, phone, audio and storage operations use synthetic fixtures and in-memory
state. No physical device, host keyboard, socket or real SD card is accessed.
The launcher is labelled SIMULATED DATA. Map tiles are a synthetic grid, not
geographic data; the original map UI still handles zoom, panning and markers.
The clock mock uses a fixed date/time. Virtual SD writes last for the process.

## Build and run

For Windows x64, download the **TWatchUltra-simulator-windows-x64** artifact
from a successful **Simulator tests** GitHub Actions run. Extract the ZIP and
double-click `twatch_desktop.exe`. This portable package includes statically
linked SDL2 and C runtime; no development tools are needed. `smoke-test.cmd`
automatically cycles all 62 entries and saves a log. CI runs all 70 desktop
tests on Windows before publishing the artifact.

Requires Git, CMake 3.16+, a C/C++17 compiler, and network access on the first
configure to fetch the pinned LVGL v9.5.0 dependency.

```sh
cmake -S simulator -B build/simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/simulator --config Release --parallel 2
ctest --test-dir build/simulator -C Release --output-on-failure
```

For the desktop launcher, install SDL2 development files (Ubuntu: `libsdl2-dev`;
Windows: SDL2 via vcpkg and its CMake toolchain), then:

```sh
cmake -S simulator -B build/desktop -DTWATCH_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/desktop --config Release --parallel 2
ctest --test-dir build/desktop -C Release --output-on-failure
./build/desktop/twatch_desktop
```

Multi-config Windows builds place the executable in `build/desktop/Release`.
The launcher uses the firmware's 410 x 502 portrait geometry. Mouse acts as
touch; drag to scroll the list. **B** returns to the launcher, **P** toggles the
simulated display, **M** toggles motion, and **Escape** exits. Each screen with
an exported stop hook is stopped when B returns to the launcher. Original
in-screen navigation remains available. Worker-driven NFC and wardriver flows
are pumped by the host runtime, as they are by the firmware main loop.

## Validation

CTest opens, renders, leaves and reopens every registered screen in isolation,
and cycles through the complete inventory in one process. Interaction tests
exercise port-scan completion, BLE clear, simulated BadUSB completion, NFC
read/write/read, NFC before/after saves, GATT discovery, PNG map decoding and
voice recording/response. Existing stopwatch, calendar, watch and storage tests
remain. Desktop builds also run `twatch_desktop --smoke` with SDL's dummy video
driver, cycling through all screens using the real SDL render path.

The tests validate host UI integration and sample workflows, not physical-device
behavior or exhaustive visual equivalence. The watch face in `main.cpp`, boot
onboarding and boot overlays are outside the dedicated-screen inventory.

PlatformIO shortcuts remain available:

```sh
pio run -e sim -t sim_build
pio run -e sim_test -t sim_check
```

The production PlatformIO target excludes `src/sim/`; no firmware screen or
hardware backend is modified by this integration.
