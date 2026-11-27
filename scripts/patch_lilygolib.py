# PlatformIO pre-build hook: apply the LilyGoLib source patches this firmware
# depends on, after lib_deps are fetched. These live inside .pio/libdeps/ (which
# is gitignored and reset by `pio run -t clean` or any library refetch), so
# without this hook a fresh checkout builds incorrectly or fails to compile.
# Idempotent and runs every build.
#
# Patches:
#   * SEND_BUF_SIZE 16384 -> 4096 in LilyGoDispInterface.cpp
#       Stock 32 KB SPI/DMA chunks fail with ESP_ERR_NO_MEM once WiFi
#       promiscuous + active BLE scan (wardriver) eat most of internal SRAM,
#       so screen redraws partially fail (wardriver->clock appears to freeze).
#       8 KB chunks fit in the SRAM left over. See README.
#   * LV_USE_SNAPSHOT 0 -> 1 in lv_conf.h
#       Required by screenshot.cpp (lv_snapshot_take); LilyGoLib ships it off.
Import("env")  # noqa: F821 (provided by the PlatformIO build environment)
import os
import re

libdeps = env.subst("$PROJECT_LIBDEPS_DIR")
pioenv  = env.subst("$PIOENV")
src_dir = os.path.join(libdeps, pioenv, "LilyGoLib", "src")

# (filename, #define name, required value)
PATCHES = [
    ("LilyGoDispInterface.cpp", "SEND_BUF_SIZE",   "(4096)"),
    ("lv_conf.h",               "LV_USE_SNAPSHOT", "1"),
]


def set_define(path, name, value):
    if not os.path.isfile(path):
        print("[patch_lilygolib] WARNING: %s not found; %s NOT patched" % (path, name))
        return
    with open(path, "r", encoding="utf-8") as f:
        src = f.read()
    pat = re.compile(r"(#define\s+" + re.escape(name) + r"\s+)(\S+)")
    m = pat.search(src)
    if not m:
        print("[patch_lilygolib] WARNING: #define %s not found in %s" % (name, os.path.basename(path)))
        return
    if m.group(2) == value:
        print("[patch_lilygolib] %s already %s" % (name, value))
        return
    src = pat.sub(lambda mm: mm.group(1) + value, src, count=1)
    with open(path, "w", encoding="utf-8") as f:
        f.write(src)
    print("[patch_lilygolib] %s: %s -> %s" % (name, m.group(2), value))


for fname, name, value in PATCHES:
    set_define(os.path.join(src_dir, fname), name, value)


# The Ultra PMU driver reads and clears edge IRQs but only publishes click/long
# click events. Publish both edges separately, preserving the existing event.
# Fail loudly if the pinned upstream source changes; never silently lose hold.
def replace_once(path, old, new):
    with open(path, encoding="utf-8") as f:
        src = f.read()
    if new in src:
        return
    if src.count(old) != 1:
        raise RuntimeError("[patch_lilygolib] unexpected power edge anchor in " + path)
    with open(path, "w", encoding="utf-8") as f:
        f.write(src.replace(old, new, 1))


replace_once(os.path.join(src_dir, "LilyGoEventManage.h"),
             "    PMU_EVENT_BAT_FET_OVER_CURRENT,\n",
             "    PMU_EVENT_BAT_FET_OVER_CURRENT,\n    PMU_EVENT_KEY_DOWN,\n    PMU_EVENT_KEY_UP,\n")
replace_once(os.path.join(src_dir, "LilyGoWatchUltra.cpp"),
             "    if (pmu.isPekeyNegativeIrq()) {\n        log_d(\"isPekeyNegative\");\n    }\n    if (pmu.isPekeyPositiveIrq()) {\n        log_d(\"isPekeyPositive\");\n    }",
             "    const bool power_key_down = pmu.isPekeyNegativeIrq();\n    const bool power_key_up = pmu.isPekeyPositiveIrq();")
replace_once(os.path.join(src_dir, "LilyGoWatchUltra.cpp"),
             "    if (event != PMU_EVENT_NONE) {\n        sendEvent(POWER_EVENT, &event);\n    }",
             "    if (power_key_down) {\n        PMUEventType_t edge = PMU_EVENT_KEY_DOWN;\n        sendEvent(POWER_EVENT, &edge);\n    }\n    if (power_key_up) {\n        PMUEventType_t edge = PMU_EVENT_KEY_UP;\n        sendEvent(POWER_EVENT, &edge);\n    }\n    if (event != PMU_EVENT_NONE) {\n        sendEvent(POWER_EVENT, &event);\n    }")

# The deep-sleep boot gate must see a press edge immediately and the release
# edge even if the user taps before the CPU has booted. The light-sleep
# routine keeps the SDK's normal short-click wake source.
replace_once(os.path.join(src_dir, "LilyGoWatchUltra.cpp"),
             '        log_d("Enable power button from wakeup source.");\n        pmu.enableIRQ(XPOWERS_AXP2101_PKEY_SHORT_IRQ);',
             '        log_d("Enable power button from wakeup source.");\n        pmu.enableIRQ(XPOWERS_AXP2101_PKEY_NEGATIVE_IRQ | XPOWERS_AXP2101_PKEY_POSITIVE_IRQ);')

# The Ultra owns and initializes global SPI. RadioLib must not reinitialize
# or end the bus shared with the SD and NFC drivers.
replace_once(os.path.join(src_dir, "LilyGoWatchUltra.h"),
             "new Module(LORA_CS,LORA_IRQ,LORA_RST,LORA_BUSY)",
             "new Module(LORA_CS,LORA_IRQ,LORA_RST,LORA_BUSY,SPI)")
