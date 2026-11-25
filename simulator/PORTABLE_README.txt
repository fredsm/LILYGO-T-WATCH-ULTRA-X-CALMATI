T-Watch Ultra desktop simulator (Windows x64)

Extract the entire ZIP, then double-click twatch_desktop.exe.
No compiler, SDL installation, watch or other hardware is required.

All 61 original dedicated firmware screens plus task manager are available.
Startup shows the original watch face at 410 x 502 pixels.
Swipe left: Wardriver; right: Meshtastic; down: Tools; up: Time.
Use the touchscreen or hold the left mouse button and drag.
F1 opens the debug screen catalog; click a screen button to inspect it.
Mouse clicks act as touch. F4 returns directly to the watch face.
Power button: click below the watch, or F2 (P when not editing).
Back button: click below the watch, or F3 (B/Escape when not editing).
F5 toggles the simulated display; M toggles simulated motion.
Ctrl+Q or the window close button exits.

Tap a firmware text field or open its compose keyboard, then type normally.
UTF-8 input, Backspace, Delete, arrows, Home/End and Ctrl+V are supported.
Enter invokes the firmware keyboard confirmation; Escape cancels editing.
Letter shortcuts never run while a text field is active.
The 60-pixel button bar is outside the unchanged 410 x 502 watch display.

All data and hardware operations are simulated in memory. Virtual SD writes
are discarded when the program closes. Map tiles show a synthetic grid.
Boot onboarding and boot overlays are not included yet.

To automatically cycle all 62 entries through the SDL renderer, double-click
smoke-test.cmd. It saves smoke-test.log and reports PASS or FAIL. The test uses
an invisible dummy video driver; run twatch_desktop.exe for interactive viewing.

CI also runs all desktop tests before publishing this package, including
screen reopening, navigation and sample NFC, BLE, portscan and voice workflows.
