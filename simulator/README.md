# Native simulator — first milestone

Requires CMake, Git and a C/C++ compiler. LVGL 9.5.0 is fetched at configure time to match firmware.

```sh
cmake -S simulator -B build/simulator -DCMAKE_BUILD_TYPE=Release
cmake --build build/simulator --config Release
ctest --test-dir build/simulator -C Release --output-on-failure
```

The initial executable is a headless rendering smoke test at 502 x 410, not a desktop watch UI. Next milestones: SDL window/input, core mock state, then original screen integration. Firmware remains the default PlatformIO target. Native code is excluded from that target.
