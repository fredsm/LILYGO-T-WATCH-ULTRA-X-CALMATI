T-Watch Ultra desktop simulator (Windows x64)

Extract the entire ZIP, then double-click twatch_desktop.exe.
No compiler, SDL installation, watch or other hardware is required.

All 61 original dedicated firmware screens plus task manager are available.
Startup shows the original skull splash, then the watch face at 410 x 502 pixels.
Swipe left: Time; right: Meshtastic; up: Tools; down: Quick Controls.
Time is the first Tools tile. Swipe down/Back from Time returns to Tools.
Use the touchscreen or hold the left mouse button and drag.
F1 opens the debug screen catalog; click a screen button to inspect it.
Mouse clicks act as touch. F4 returns directly to the watch face.
Power button: click below the watch, or F2 (P when not editing).
Back button: click below the watch, or F3 (B/Escape when not editing).
F5 toggles the simulated display; F6 opens the hardware simulation panel.
Ctrl+Q or the window close button exits.

Tap a firmware text field or open its compose keyboard, then type normally.
UTF-8 input, Backspace, Delete, arrows, Home/End and Ctrl+V are supported.
Enter invokes the firmware keyboard confirmation; Escape cancels editing.
Letter shortcuts never run while a text field is active.
The 60-pixel button bar is outside the unchanged 410 x 502 watch display.

All data and hardware operations are simulated in memory. Virtual SD writes
are discarded when the program closes. Map tiles show a synthetic grid.
The separate hardware panel controls battery, charging, full-range brightness,
display power, GPS fix/satellites and virtual SD presence. Full brightness is the
default. Panel controls stay usable even when the watch is off. Close hides the
panel; F6 reopens it. Clock battery updates take about one second. GPS and SD
changes appear when the original screens refresh or reopen.

Double-click onboarding.cmd to include the original optional onboarding screen.
Acceptance is session-only; no email, consent or telemetry is saved/sent.
Run twatch_desktop.exe --skip-boot to skip the 1.5-second splash.

To automatically cycle all 63 entries through the SDL renderer, double-click
smoke-test.cmd. It saves smoke-test.log and reports PASS or FAIL. The test uses
an invisible dummy video driver; run twatch_desktop.exe for interactive viewing.

CI also runs all desktop tests before publishing this package, including
screen reopening, navigation and sample NFC, BLE, portscan and voice workflows.

Double-click interaction-test.cmd for the SDL navigation, keyboard, touch-button
and hardware-panel regression checks. Results are saved in interaction-test.log.

Both simulator and physical firmware UI are English, including onboarding
and status messages. Auto is now Car; Credito is NFC Compare.

Quick Controls toggles Wi-Fi, Bluetooth, GPS, LoRa and NFC. Brightness cycles
33%, 66%, 100% and saves through Settings. Swipe up, Back or Power closes it.
These navigation and control handlers are shared with the physical firmware.
Bluetooth/LoRa stay on while another tool owns them; the UI explains why.

Quick Controls includes Settings and Lock shortcuts beneath the radio tiles.
Lock returns to the clock with a green padlock. Tap or press Power/Back to
open the numeric keypad; enter 1234 to unlock.
The existing Back-from-home Settings shortcut remains available.

Wardriver is the second Tools tile, after Time. Home swipe right-to-left opens
Time. Back or a rightward/downward swipe from Time returns to the screen that
opened it (home or Tools). Wardriver Back/right swipe returns to Tools.

Full power-off: hold Power (or F2) for five seconds until the watch blanks,
then release. Only another five-second Power hold wakes it, showing a simulated boot.
The hardware panel and F5 cannot wake a powered-off watch.
ScreenOff Timer in Settings defaults to 60 secs (Off / 60 secs / 3000 secs).
A short Power press wakes normal screen-off without changing pages.

CPU Frequency: choose 80 / 160 / 240 MHz (default) in Settings. The simulated
hardware panel shows the current frequency; this does not change the host CPU
or emulate actual watch timing/current draw. No automatic scaling is enabled.
