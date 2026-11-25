# UI simulator

The goal is original LVGL UI development, not hardware emulation. Real RF, peripherals, storage behavior and performance are tested on the watch.

Headless build (CMake, Git, C/C++ compiler):
```sh
cmake -S simulator -B build/simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/simulator --config Release
ctest --test-dir build/simulator -C Release --output-on-failure
```

Optional desktop preview requires SDL2 development package with SDL2 CMake config (Ubuntu: libsdl2-dev; Windows: SDL2 via vcpkg and its CMake toolchain).
```sh
cmake -S simulator -B build/desktop -DTWATCH_DESKTOP=ON -DCMAKE_BUILD_TYPE=Release
cmake --build build/desktop --config Release
```
Run twatch_desktop (on Windows typically Release/twatch_desktop.exe). Mouse acts as touch. P toggles display, B logs back button, M toggles motion, Escape exits. This is a temporary runtime test screen, not the original UI. Next integrate original screens with minimal mock adapters. No real radio or network operations.
