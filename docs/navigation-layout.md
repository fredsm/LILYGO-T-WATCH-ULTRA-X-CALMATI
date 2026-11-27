# Configurable navigation layout

This is a presentation layer for **already compiled, built-in screens**, not an
add-on loader or user-defined tool engine. It does not install or execute code,
start services at boot, or change the existing behavior of any tool.

## Configuration file

Copy [sd/Settings/layout.txt](../sd/Settings/layout.txt) onto the SD card at
`/Settings/layout.txt`. It is optional. The firmware reads it once after storage
initialization, before constructing the menus. Edit it on a PC and restart the
watch using the normal power-off/wake mechanism. No firmware rebuild is required.

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

The example above reproduces the current default layout. Direction names describe
the movement of the finger, not the side from which the swipe starts.

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
  Listed items move to the front of the two-column grid; any unlisted items
  retain their default relative order. This lets a short list act as favorites.
- `tools.hide`: exact existing tile labels to hide from this grid. Does not
  disable a service or remove its implementation. An empty value hides none.
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
starting with `#`, CRLF/LF and a UTF-8 BOM. Missing keys use factory defaults.
Invalid keys, destinations, duplicate keys or list entries, unknown tile labels,
malformed syntax, read failures and unsafe Settings reachability reject the
entire file. If there is no SD card/file, the watch uses built-in defaults.
No writeback to SD occurs. Diagnostics reports whether the layout came from
the SD file, built-in defaults, or an invalid file with defaults restored.

To recover, remove or correct the file and reboot. No changes are applied on
card insertion, removal or while the UI is running.

## Desktop simulator

The simulator reads the same parser and applies the same layout code.
Launch the packaged desktop executable with:

```
twatch_desktop.exe --layout layout.txt
```

The explicit local file is copied to the simulator's virtual SD, and is read
during simulated boot (real files are not used as watch SD storage). Without
the argument, default mappings apply. A simulated deep reboot reloads the
virtual file and restores menu positions; light sleep retains the current
layout. The simulator's screen catalog remains a developer/testing interface
independent of the configured watch menu.

## Hardware acceptance

Check both a missing file and a valid custom file; verify each Home direction
and both short presses, reorder/hide Tools and Quick Controls, exercise PIN
lock/wake/long-press shutdown, and verify existing screen exit behavior.
Test malformed file recovery and card removal/reboot. The actual SD and
touch/button timing still require watch testing.
