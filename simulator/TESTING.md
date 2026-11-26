# Simulator acceptance checks

Use the Windows package from the successful CI run linked in UI_COVERAGE.md.
Extract the entire ZIP before running it. These checks use synthetic data only.

## Automated checks

- `smoke-test.cmd`: render all 62 catalog entries in one process.
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
| Swipe left/right/up/down from the clock | Wardriver / Meshtastic / Time / Tools, respectively. F4 always returns to the clock after startup. |
| Swipe up inside Tools | Tiles scroll. A downward swipe at the top edge returns to the clock. |
| Open Time, tap Stopwatch, then start/stop/reset | Original stopwatch controls respond. F4 returns home. |
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
