# Configurable navigation layout

This is a presentation layer for **already compiled, built-in screens**, not an
add-on loader or user-defined tool engine. It does not install or execute code,
start services at boot, or change the existing behavior of any tool.

## Configuration file

Copy [sd/Settings/layout_default.txt](../sd/Settings/layout_default.txt) to
`/Settings/layout_default.txt` on the SD card. Copy it to `/Settings/layout.txt`
if you want to customize your watch. Both are optional.

Boot loads the tiers in this order (each valid file overrides the preceding
tier):

1. Embedded copy of the canonical `layout_default.txt`, generated at build time.
2. SD `/Settings/layout_default.txt`.
3. SD `/Settings/layout.txt` (custom layout).

Missing keys inherit from the previous tier. A malformed file is rejected as a
whole; it does not discard a valid earlier tier. The canonical repository file
is the single source of truth for the standard layout; PlatformIO and CMake
regenerate the same embedded header when it changes. There is no second
hardcoded normal layout. A minimal Settings/Tools recovery mapping exists only
in case a malformed canonical file was built; the simulator regression test
requires the embedded file to parse successfully.

The firmware reads the files once after storage initialization, before
constructing the menus. Edit them on a PC and restart the watch. No firmware
rebuild is required for SD edits.

```ini
home.swipe_left = time
home.swipe_right = meshtastic
home.swipe_up = tools
home.swipe_down = quick_controls
home.back = settings
home.power = gps
tools.order = Time, Wardriver, Scanner
tools.hide =
quick.controls = wifi, bluetooth, gps, lora, nfc, brightness
quick.actions = settings, lock
```

The shorter example above reproduces the current default layout; the canonical
file lists the full Tools order. Direction names describe
the movement of the finger, not the side from which the swipe starts.

## Screen inventory

The canonical file includes two informational, comma-separated fields:
`available-screens` lists all built-in dedicated screens, including internal
screens, Home, onboarding and the PIN lock. `available-home-targets` lists
the supported direct Home mappings, including `none`. Changing these
reference lists does not register, unlock, hide or remap screens. Use
`tools.order` to arrange existing tool tiles and the Home keys to select
supported shortcuts. The inventory is checked by a regression test.

## Home destinations

Each of the six Home inputs accepts one value:

| ID | Opens |
|---|---|
| `none` | Nothing |
| `time` | Time menu |
| `meshtastic` | Meshtastic screen |
| `tools` | Built-in Tools menu |
| `quick_controls` | Quick Controls |
| `settings` | Settings |
| `gps` | GPS status |
| `lora` | LoRa status |
| `wifi_radio` | Wi-Fi radio status |
| `bluetooth` | Bluetooth status |
| `nfc` | NFC status |
| `calendar` | Calendar |
| `stopwatch` | Stopwatch |
| `timer` | Timer |
| `alarm` | Alarm |
| `diagnostics` | System Diagnostics |
| `task_manager` | Task Manager |

For example, swap the Time and Tools shortcuts, put diagnostics on Back, and
Settings on Power:

```ini
home.swipe_left = tools
home.swipe_up = time
home.back = diagnostics
home.power = settings
```

Only the Home/clock mappings change. Inside an existing screen, gestures and
button behavior retain their current semantics, including the radio and
Meshtastic chains, returning from tools, editing, and cleanup callbacks.
Wake presses, PIN unlock, five-second shutdown/wake, and boot onboarding are
not configurable.

## Menus

- `tools.order`: exact, case-sensitive existing tile labels, comma separated.
  Listed items move to the front of the two-column grid. All unlisted built-in
  tools are appended at the end in the previous tier’s relative order, even
  when new built-in tools are added to the firmware. Only tools explicitly
  hidden by `tools.hide` are not shown. This lets a short list act as favorites.
- `tools.hide`: exact existing tile labels to hide from this grid. Does not
  disable a service or remove its implementation. An empty value clears inherited hiding and hides none.
- `quick.controls`: exact order of visible large tiles. Available IDs:
  `wifi, bluetooth, gps, lora, nfc, brightness`. Unlisted tiles are hidden.
- `quick.actions`: exact order of visible bottom buttons: `settings, lock`.
  Unlisted buttons are hidden. The Settings and Lock behaviors are unchanged.

At least one Home mapping must open Settings directly, or open Quick Controls
with its Settings button visible. This avoids making the settings inaccessible.
If you hide an existing tool, its existing internal navigation remains intact;
this is not an access-control mechanism.

## Validation and recovery

The file is limited to 4096 bytes and accepts blank lines, spaces, comments
starting with `#`, CRLF/LF and a UTF-8 BOM. Missing keys inherit from the previous tier.
Invalid keys, destinations, duplicate keys or list entries, unknown tile labels,
malformed syntax, read failures and unsafe Settings reachability reject the
entire affected file. If both SD files are missing or invalid, the watch uses
the embedded canonical defaults. No writeback to SD occurs. Diagnostics reports
the effective source (Custom SD, Default SD, Embedded defaults) and any rejected
files.

To restore the SD default layout, remove or rename the custom layout.txt and
reboot. To restore the embedded default, remove or rename both SD layout files. No changes are applied on
card insertion, removal or while the UI is running.

## Desktop simulator

The simulator reads the same parser and applies the same layout code.
Launch the packaged desktop executable with:

```
twatch_desktop.exe --layout layout.txt
twatch_desktop.exe --layout-default layout_default.txt --layout layout.txt
```

The explicitly supplied local files are copied to the simulator's virtual SD
and read during simulated boot (real files are not used as watch SD storage).
Without arguments, embedded defaults apply. A simulated deep reboot reloads
the virtual files and restores menu positions; light sleep retains the current
layout. The simulator's screen catalog remains a developer/testing interface
independent of the configured watch menu.

## Hardware acceptance

Check both a missing file and a valid custom file; verify each Home direction
and both short presses, reorder/hide Tools and Quick Controls, exercise PIN
lock/wake/long-press shutdown, and verify existing screen exit behavior.
Test malformed file recovery and card removal/reboot. The actual SD and
touch/button timing still require watch testing.
