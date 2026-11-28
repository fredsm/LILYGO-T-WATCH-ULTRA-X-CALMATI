# Configurable navigation layout

This is a presentation layer for **already compiled, built-in screens**, not an
add-on loader or user-defined tool engine. It does not install or execute code,
start services at boot, or change the existing behavior of any tool.

## Configuration storage

There are two active layout sources:

| Location | File | Role |
|---|---|---|
| Firmware application flash | embedded `layout_default.txt` | Factory default |
| Internal FFat | `/Settings/layout.txt` | Customized layout |

[config/layout_default.txt](../config/layout_default.txt) is the single
canonical default; PlatformIO and CMake embed it at build time. On boot the
watch reads the internal layout, falls back to the last valid internal backup
if the primary is missing/corrupted, and otherwise uses the embedded default.
Missing keys in a custom layout inherit from the embedded default. SD files
are **never** consulted on boot; external SD is only a transfer workspace.

### Settings > Layout Manager

- **Export current layout to SD** writes the active internal layout (or the
  embedded default if no custom layout is active) to SD `/Settings/layout.txt`.
- **Export default layout to SD** writes the canonical firmware default to SD
  `/Settings/layout_default.txt`.
- **Import layout from SD** reads SD `/Settings/layout.txt`, validates it
  against the embedded default, verifies an internal temporary file and saves
  it to FFat. Successful import immediately applies the layout and persists
  across reboots and deep sleep. SD `layout_default.txt` is a reference only;
  rename/copy it to `layout.txt` before import.
- **Restore default layout** requires two taps and removes only the layout
  manager's internal primary/backup/temp files. It immediately applies the
  embedded default. It does not erase SD or other internal files.

Export stages and verifies the new file before publication; an existing SD
editing copy is preserved as `layout_export.bak` (or `layout_default.bak`).
Keep your own backups before overwriting these paths. Eject the USB SD drive
before importing or exporting; the normal SD ownership guard refuses transfers
when USB owns the card.

FFat mounts without automatic formatting. If unformatted/unmountable, the
watch remains usable with the embedded default. Settings shows **Initialize
internal storage** only when FFat is unavailable. This retries the mount;
after explicit two-tap confirmation it may format the internal FAT partition.
**Formatting can erase all internal FAT files**, so only confirm after backup
or on a new/unformatted device. The application, NVS, SD and voice-model
partitions are not formatted by this action. The legacy vendor MSC startup is
disabled because it can auto-format and expose FFat; the firmware's existing
SD-only USB reader remains enabled.

Manual import/export/initialization are explicit synchronous UI actions rather
than background polling. Native storage latency (especially formatting) is not
bounded by the byte limit. Remote downloads and their SD/FFat transactions run
in a background worker; only the UI owner applies the resulting layout.

### Remote configuration

1. Host a plain-text layout file at a direct URL (for GitHub, use the **Raw**
   file URL, not the HTML repository view).
2. Connect the watch to Wi-Fi, insert the SD card, and eject/stop USB SD mode.
3. Open Settings > Layout Manager > **Layout Source URL**, enter the URL
   (up to 512 characters), and tap Save URL. It is stored in internal NVS,
   independent of the SD card or the FFat partition. Save an empty field to
   clear the URL. Avoid putting credentials or access tokens in URLs.
4. **Download layout to SD** validates and replaces SD `/Settings/layout.txt`
   without touching the active layout. Use Import layout from SD when ready.
5. **Download & Apply Layout** performs the same verified SD transfer, then
   saves the validated bytes to internal FFat and applies the layout immediately.
   If the internal write fails, the downloaded SD copy remains available for
   retry with Import layout from SD; the previous active layout is retained.

HTTPS verifies the server certificate and hostname with the SDK's trusted CA
bundle (requires an accurate clock); HTTP is supported for trusted local
servers but is unencrypted. Redirects are rejected: use the final direct URL.
Only HTTP 200 and complete, nonempty bodies up to 4096 bytes are accepted
(content length or chunked framing required). The worker uses a five-second
socket timeout and checks a 20-second deadline between network operations.
No network request occurs on boot, and there are no scheduled downloads.
Other layout actions are disabled during a transfer. The existing radio
manager serializes downloads with other managed network services; a busy
radio or unavailable SD results in a visible retry message.

The desktop simulator exercises the same URL storage, validation, transfer and
application code with a synthetic HTTP response (`home.power=timer` by default).
It does not make real web requests. Regression tests inject invalid, oversized,
incomplete and failed responses.

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
  tools are appended at the end in the embedded default’s relative order, even
  when new built-in tools are added to the firmware. Only tools explicitly
  hidden by `tools.hide` are not shown. This lets a short list act as favorites.
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
starting with `#`, CRLF/LF and a UTF-8 BOM. Missing keys inherit from the
embedded default. Invalid keys, destinations, duplicate keys or list entries,
unknown tile labels, malformed syntax, read failures and unsafe Settings
reachability reject the entire candidate. Import never publishes an invalid
file. Internal saves use a verified temporary file plus a previous-good
backup; boot ignores an unfinished temporary file. If both internal files are
missing/invalid, embedded defaults remain usable.

Diagnostics reports effective source (Internal flash, Internal backup or
Embedded defaults), mount availability and invalid internal file recovery.
No layout is reloaded on SD insertion/removal or USB eject. Use explicit
import after editing, or Restore default layout to clear the internal override.

## Desktop simulator

The same parser, Layout Manager and persistence logic run against separate
virtual SD and internal FAT filesystems. Launch with an explicitly supplied
desktop fixture for testing:

```
twatch_desktop.exe --layout layout.txt
```

This validates and saves the file into virtual internal flash, not virtual SD.
Internal state survives simulated deep reboot and is session-only; the simulator
does not write to host files. The catalog remains a developer/testing
interface independent of the configured watch layout.

## Hardware acceptance

On the actual watch, check mounting FFat without auto-format; only initialize if
appropriate. Export the current or default layout, edit the SD copy on a PC,
eject USB SD, import and verify the layout changes immediately. Remove SD and
reboot/deep sleep; the custom layout should remain. Verify invalid import and
interrupted-write recovery, restore defaults, and confirm USB storage exposes
only SD, not internal flash. Real FFat/USB timing and failure behavior remain
physical-watch validation items.
