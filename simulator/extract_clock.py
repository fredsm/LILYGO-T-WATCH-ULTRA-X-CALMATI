"""Compile the firmware's clock UI on the host without compiling setup/loop.

Only verbatim source slices are emitted. Missing/ambiguous anchors fail the
build, rather than silently falling back to a separately maintained clock UI.
Production sources are never rewritten by this script.
"""
import argparse
from pathlib import Path
import re

FUNCTIONS = (
    'bat_color', 'build_battery_widget', 'build_analog_clock',
    'update_defcon_countdown', 'update_analog_clock', 'build_lora_indicator',
    'realign_status_icons', 'update_battery', 'layout_battery_indicators',
    'build_clock_icon', 'resize_clock_text', 'update_clock',
    'clock_screen_set_analog_face', 'clock_screen_set_12h',
    'clock_screen_uses_12h', 'clock_screen_set_show_day',
    'clock_screen_set_show_date', 'clock_screen_set_show_ampm',
    'clock_screen_set_show_secs', 'clock_screen_set_vibrate',
    'clock_screen_set_matrix',
    'on_clock_gesture',
)

def between(source, start, end):
    if source.count(start) != 1 or source.count(end) != 1:
        raise ValueError(f'Ambiguous/missing clock boundary: {start!r}, {end!r}')
    return source[source.index(start):source.index(end)]

def function(source, name):
    # Mask comments and literals while preserving offsets for brace matching.
    masked = re.sub(r'//[^\n]*|/\*[\s\S]*?\*/|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'',
                    lambda m: ' ' * len(m[0]), source)
    matches = list(re.finditer(r'^\s*(?:static\s+)?[\w:* ]+\b' + re.escape(name)
                              + r'\s*\([^;{}]*\)\s*\{', masked, re.M))
    if len(matches) != 1:
        raise ValueError(f'Expected one definition of {name}, found {len(matches)}')
    start = matches[0].start()
    opening = masked.index('{', start)
    depth = 1
    end = opening + 1
    while depth and end < len(masked):
        depth += (masked[end] == '{') - (masked[end] == '}')
        end += 1
    if depth:
        raise ValueError(f'Unbalanced function: {name}')
    return source[start:end]

def extract(source):
    headers = re.findall(r'^#include "[^"\n]*_screen\.h"', source, re.M)
    parts = ['\n'.join(headers),
             between(source, 'static lv_obj_t *clock_screen;', 'static lv_color_t bat_color'),
             between(source, '#define CLOCK_TEXT_PAD_X', 'static void resize_clock_text()'),
             'static void update_clock();\n']
    parts.extend(function(source, name) for name in FUNCTIONS)
    layout = between(source, '    clock_screen = lv_obj_create(NULL);', '    gps_screen_create();')
    parts.append('static void create_original_clock() {\n' + layout + '\n}\n')
    power = between(source, '            if (lv_screen_active() == clock_screen)\n',
                    '\n        }\n    }, POWER_EVENT, NULL);')
    back = between(source, '            if (settings_screen_is_active()) {',
                   '\n        }\n    }\n\n    // Feed NMEA')
    parts.append('static void original_power_click() {\n' + power + '\n}\n')
    parts.append('static void original_back_click() {\n' + back + '\n}\n')
    # Reuse the production splash layout and brand; host timing is nonblocking.
    brand = between(source, '#define FW_NAME', '\nvoid setup()')
    splash = between(source, '    lv_obj_t *boot_splash = lv_obj_create(NULL);',
                     '    lv_refr_now(NULL);                       // paint now;')
    parts.append(brand + '\nlv_obj_t* sim::create_boot_splash() {\n' + splash
                 + '\nreturn boot_splash;\n}\n')
    return '// Generated from src/main.cpp; do not edit.\n' + '\n'.join(parts)

if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('source', type=Path)
    parser.add_argument('output', type=Path)
    args = parser.parse_args()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(extract(args.source.read_text(encoding='utf-8')), encoding='utf-8')
