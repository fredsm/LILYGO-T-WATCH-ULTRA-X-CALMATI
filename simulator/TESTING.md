# Simulator acceptance checks

Use the Windows package from the successful CI run linked in UI_COVERAGE.md.
Extract the entire ZIP before running it. These checks use synthetic data only.

## Automated checks

- `smoke-test.cmd`: render all 63 catalog entries in one process.
- `interaction-test.cmd`: exercise startup and the SDL input path, including
  navigation, touch-button input, keyboard editing and the hardware panel.
- Both scripts run without visible windows, save a log beside the executable,
  report PASS/FAIL and return a nonzero exit code for a failure.
- CI additionally tests the original onboarding callbacks, consent validation,
  splash timing, battery display updates, virtual SD removal/reinsertion,
  firmware button routes, UTF-8 editing and representative original app actions.

## Hands-on touchscreen check

| Action | Expected result |
| --- | --- |
| Run `twatch_desktop.exe` | Original skull fades in, then clock appears after about 1.5 seconds. Watch area is 410 x 502; brightness starts at 100%. |
| Swipe left/right/up/down from the clock | Time / Meshtastic / Tools / Quick Controls, respectively. F4 always returns to the clock after startup. |
| Swipe up inside Tools | Tiles scroll. A downward swipe at the top edge returns to the clock. |
| Open Tools, tap its first tile (Time), then Stopwatch, then start/stop/reset | Original stopwatch controls respond. F4 returns home. |
| Tap Power and Back below the watch | Original firmware button routes execute. F2 and F3 do the same. |
| Change battery and charging in the separate panel | The original clock reflects battery changes within about one second. |
| Lower brightness, toggle watch display off, then on | Watch dims/blanks; panel and physical buttons remain usable. |
| Toggle GPS fix/satellite count with GPS enabled | Original GPS screen reflects the synthetic readings on its next refresh. |
| Remove virtual SD, then reinsert it | Card-presence consumers see removal; virtual file reads/writes fail while absent. Existing in-memory contents remain after reinsertion. |
| Close hardware panel, press F6 in watch window | Watch continues; panel reopens. |
| Open a message compose field and type | Text enters the original editor. Backspace edits, Enter confirms, Escape cancels without quitting. |
| Run `onboarding.cmd` | Original optional onboarding appears after splash. Accept requires the checkbox, then opens the clock. No real email is needed; nothing is sent or persisted. |
| Press Ctrl+Q or close the watch window | Both windows exit. |

All automated checks validate host integration. They do not establish physical
radio behavior, real sensor timing, or exhaustive visual/touchscreen equivalence.

Quick Controls acceptance: toggle each radio twice, verify its original status
page agrees, and cycle brightness through all three levels. Time must stay the
first Tools tile; Back/down from Time returns to Tools.

## Shared firmware controls

The simulator now compiles the production Quick Controls screen and uses the
production home gesture and physical-button handlers. Navigation tests also
check Bluetooth stack ownership, LoRa pager ownership, and brightness settings
persistence. Firmware CI compiles the actual ESP32 target separately.

On the physical watch, check each shortcut against its status screen and home
icon. Verify Time is first in Tools, Time returns to Tools, Settings opens from
Quick Controls, and brightness survives dim/wake (and reboot with writable SD).
Repeat the Power/Back radio-screen cycle and left/right home swipes. These
on-device checks are still required; host tests cannot verify radio hardware.

Wardriver is the second Tools tile, after Time. Home swipe right-to-left opens
Time. Back or a rightward/downward swipe from Time returns to the screen that
opened it (home or Tools). Wardriver Back/right swipe returns to Tools.

## Power-off and wake

- Hold the physical Power/crown button for five seconds. The display goes
  black and touch navigation is locked; release to complete deep sleep.
- Touch, Back, motion, timers and USB insertion are not configured as wake
  sources. A new five-second hold of Power boots the watch; shorter taps remain off. Check on battery and
  when connected to USB, and verify the RTC retains time.
- Short Power clicks still use the original screen routes. The PMIC emergency
  hardware cutoff is configured for ten seconds (release after the five-second
  blanking threshold for normal deep sleep).
- Eject USB mass storage on the computer before powering off to avoid losing
  host-cached writes. The watch stops local recording and logging before SD
  unmount. The library's hardware shutdown sequence may take a few seconds.
- On desktop, hold F2 (or the onscreen Power button) for five seconds, release,
  and hold Power for five seconds again to see a simulated boot. F3/F4/F5, touch and the debug
  hardware display toggle cannot wake deep sleep. The desktop models the UI
  and wake policy, not physical current draw or a complete MCU memory reset.
- `power_button_hold` tests timing boundaries, repeat edges and timer wrap;
  `power_manager_shutdown` tests shared state transitions. The SDL end-to-end
  test also holds the simulated physical button and verifies power-only wake.

## Screen-off timer and wake
- Settings > ScreenOff Timer: Off, 60 secs (default), or 3000 secs (50 minutes).
  The selection is saved with other settings on a writable SD card.
- Touch or button activity resets the independent inactivity timer. Motion still
  resets only the dim timer. With the panel off, touch and Back cannot navigate.
- A short Power press wakes the panel at the configured brightness, preserving
  the active screen; the wake press is not also a navigation click.
- The screen-off timer does not suspend background services. The reusable
  lightsleep() path also wakes on Power; automatic light-sleep scheduling and
  a dedicated lock-screen UI are not enabled yet.
- Verify physical five-second wake qualification on battery and USB: the PMIC
  wakes the MCU on the press edge, but the screen stays dark until the hold
  qualifies. A short tap returns to deep sleep without normal boot.

## PIN lock
- Swipe down on the clock and tap **Lock** next to Settings. The watch stays
  on the clock with a green padlock at the top; touch or either physical
  button opens the numeric unlock screen without navigating elsewhere.
- The PIN is currently hardcoded to `1234`. Four digits submit automatically;
  wrong PINs clear the masked entry and display an error. Clear and Backspace
  are available. The opening touch must not enter a digit.
- After correct entry the watch returns to its clock with the gray indicator.
  PIN lock is separate from duress and from the temporary sleep input block.
- The screen-off timer still runs while locked; it discards partial PIN digits.
  Power wakes the keypad, and a five-second shutdown remains available.
  Internal NVS retains the lock across a reboot (desktop NVS is session-only).
- `pin_lock_authentication` and the SDL end-to-end test exercise these flows.

## CPU frequency control
- Settings > CPU Frequency: 80 MHz, 160 MHz, 240 MHz (default), or Auto. Changes
  apply immediately; the chosen rate is stored in internal NVS, not the SD.
- The hardware panel displays the simulated active CPU frequency. The mock
  models the selected clock state, not host CPU speed, watch timing or current draw.
- Runtime helpers can step up/down across the three rates and restore the saved
  preference without changing it. Invalid rates are rejected. In fixed mode,
  lock, screen-off and light sleep do not change the chosen rate. Auto continues
  while screen-off, pauses during sleep and resumes at 240 MHz on light wake.
- Test reduced-rate performance with LVGL, Wi-Fi/BLE, LoRa and active tools on
  actual hardware. Auto samples per-core scheduler occupancy every 500 ms;
  the busiest core determines the target (>80% = 240 MHz, >50% = 160 MHz,
  otherwise 80 MHz). Upward transitions are direct, downward transitions
  require the same lower target for five seconds. Fixed settings disable
  the governor. In the mock, use the core-demand sliders in the hardware
  panel to exercise the same policy. Auto persists across reboot.

## System Diagnostics (Phase 1)
- Open **Tools > Task Mgr > Diagnostics**; Back returns to Task Mgr.
- Monitor the current CPU frequency, both core occupancy estimates (including
  fixed 240 MHz), UI handler duration, largest UI service gap, recent stalls,
  slowest main-loop operations, internal free/minimum/largest heap, free
  PSRAM, UI stack headroom, uptime and reset reason.
- Firmware: the heartbeat task runs independently of LVGL on core 0, checking
  every 50 ms for UI service gaps >=100 ms. Latest 12 stalls live in a fixed
  RAM buffer (screen shows five); it records recovery and likely active
  operation. No SD logging or hardware bus access from the monitor. A separate
  low-priority telemetry task samples heap and attempts best-effort USB serial
  reports. USB issues cannot block the heartbeat task. Sampling is approximate;
  short or invalid CPU sample windows can show N/A.
- Operation timings include all blocking work within the named main-loop call.
  QSPI flush/wait timing is captured using the LVGL display events. These are
  not individual electrical SPI/I2C transaction timings. Background worker
  internals and callback internals are not separately timed in Phase 1.
- Screen-off continues monitoring the UI service loop; intentional shutdown and
  sleep pause stall detection. RAM-only history is reset on reboot.
- Desktop: F6 hardware panel > **Simulate 30s SD stall** injects a recovered
  Wardriver/SD stall without actually freezing or advancing the watch clock.
  Core demand sliders work in fixed and Auto modes; memory values are fixtures.
  `diagnostics_monitor` exercises ongoing/recovered stalls, rollover history,
  metrics, pause/resume, reset, and the Task Manager entry point.
