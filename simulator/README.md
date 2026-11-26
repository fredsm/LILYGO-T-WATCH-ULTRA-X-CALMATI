# Original firmware UI simulator

## Watch-flow integration: Phases 1-6

The desktop starts with the original skull splash, then the original clock face at 410 x 502 pixels.
**F1** opens the debug screen catalog; **F4** and firmware clock-return callbacks
return to the watch face. The original analog/digital and time-format settings
now control the face. Weather is a host-only synthetic fixture, populated by
tapping the original weather label; the RTC advances from a fixed test epoch.

`extract_clock.py` selects the original layout and 22 drawing/formatting/navigation
functions verbatim from `src/main.cpp` during the host build. Missing or
ambiguous source boundaries fail the build. The production file is unchanged.
The original DEF CON image, clock font and Matrix background are compiled too.

Clock swipes reuse the original handler: left to Wardriver, right to Meshtastic,
down to Tools, up to Time. Original destination-screen gestures and tile
callbacks handle subsequent navigation. Mouse dragging and native touchscreen
finger input use the LVGL pointer driver, with the firmware scroll threshold.
Power and Back buttons below the watch display reuse the firmware button
routes (including the 250 ms Back debounce). The watch viewport remains
410 x 502; the 60-pixel control bar is outside it. A separate hardware
simulation panel opens beside it. Its battery, charging, brightness, display power,
GPS fix/satellite count and virtual SD-presence controls share state with the
firmware mocks. Battery changes appear on the clock at its next update (about
one second); GPS/SD consumers refresh according to their original screen logic.
The display defaults to full brightness. The panel and physical button bar
remain bright when the watch is dimmed or off. F6 reopens a closed panel.

The original splash layout/art is reused with a host fade-in and a 1.5-second
minimum display time. `--skip-boot` goes straight to the clock; `--onboarding`
shows the original first-use screen after the splash. Onboarding acceptance is
session-only, and telemetry is disabled in the original source. No email or
consent is saved to disk. The original UI is compiled directly, not recreated.
CI validates the integrated phases. Download its successful Windows
artifact; previously downloaded EXEs do not contain these changes.

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
automatically cycles all 62 entries and saves a log. CI runs all desktop
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
touch; drag to scroll the list. **F2/P** presses Power, **F3/B** presses Back, **F4** returns home,
**F5** toggles the display, and **F6** opens the hardware panel. **Ctrl+Q** exits.
Printable shortcuts are disabled while editing. UTF-8 typing, Backspace,
Delete, arrows, Home/End and Ctrl+V edit the active firmware text field.
Enter invokes the original keyboard Ready action; Escape cancels editing
or presses Back when no editor is active. Each screen with
an exported stop hook is stopped when the debug home action returns to the clock. Original
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
behavior or exhaustive visual equivalence. Startup tests verify the original
splash, fade timing, optional onboarding, consent and return to the clock.
`--self-test` injects SDL mouse, native finger and keyboard events through the
real event loop: four swipe routes and returns, physical Power while the watch
is off, hardware panel updates and focus isolation, panel close/reopen, catalog,
text entry, Backspace, Escape and return home. `interaction-test.cmd` runs this
check in the Windows package and saves its results. Manual touchscreen feel and
physical hardware behavior still require testing on the respective devices.

PlatformIO shortcuts remain available:

```sh
pio run -e sim -t sim_build
pio run -e sim_test -t sim_check
```

With desktop dependencies installed, build and launch in one command:

```sh
pio run -e sim -t sim_run
```

Local PlatformIO is optional: the Windows CI artifact is the supported route
for testing on machines without a desktop compiler, CMake or SDL2 development
files. Intermediate implementation commits use `[skip ci]`; a final commit
without that marker runs the Linux/Windows tests and packages the Windows EXE.

The production PlatformIO target excludes `src/sim/`; no firmware screen or
hardware backend is modified by this integration.
