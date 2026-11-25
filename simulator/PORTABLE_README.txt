T-Watch Ultra desktop simulator (Windows x64)

Extract the entire ZIP, then double-click twatch_desktop.exe.
No compiler, SDL installation, watch or other hardware is required.

All 61 original dedicated firmware screens plus task manager are available.
Click a screen button. Hold the left mouse button and drag to scroll the list.
Mouse clicks act as touch. B returns to the launcher, Escape exits.
P toggles the simulated display; M toggles simulated motion.

All data and hardware operations are simulated in memory. Virtual SD writes
are discarded when the program closes. Map tiles show a synthetic grid.
The firmware main watch face, boot onboarding and boot overlays are excluded.

To automatically cycle all 62 entries through the SDL renderer, double-click
smoke-test.cmd. It saves smoke-test.log and reports PASS or FAIL. The test uses
an invisible dummy video driver; run twatch_desktop.exe for interactive viewing.

CI also runs all 70 desktop tests before publishing this package, including
screen reopening, navigation and sample NFC, BLE, portscan and voice workflows.
