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

# Phase 5: IRQ handlers only set pending bits. Do not consume a pending
# event on a failed I2C read; retry after the manager's bounded cooldown.
ultra = os.path.join(src_dir, "LilyGoWatchUltra.cpp")
replace_once(ultra, '#include "SensorWireHelper.h"',
             '#include "SensorWireHelper.h"\n'
             'extern "C" bool twatch_i2c_ready(uint16_t address);\n'
             'extern "C" uint32_t twatch_i2c_failures();\n'
             'static uint8_t twatch_touch_address = 0x5A;\n'
             'static bool twatch_touch_fault = false, twatch_touch_discard = false;\n'
             'extern "C" bool twatch_i2c_touch_failed() { return twatch_touch_fault; }')
replace_once(ultra, '    touch.setPins(TP_RST, TP_INT);',
             '    twatch_touch_address = touch_panel_addr;\n    touch.setPins(TP_RST, TP_INT);')
replace_once(ultra,
             'uint8_t LilyGoUltra::getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point )\n{',
             'uint8_t LilyGoUltra::getPoint(int16_t *x_array, int16_t *y_array, uint8_t get_point )\n{\n'
             '    twatch_touch_fault = false;')
replace_once(ultra,
             '        uint8_t tp = touch.getPoint(x_array, y_array, get_point);',
             '        if (!twatch_i2c_ready(twatch_touch_address)) {\n'
             '            twatch_touch_fault = twatch_touch_discard = true; return 0;\n'
             '        }\n'
             '        const auto before = twatch_i2c_failures();\n'
             '        uint8_t tp = touch.getPoint(x_array, y_array, get_point);\n'
             '        if (twatch_i2c_failures() != before) {\n'
             '            twatch_touch_fault = twatch_touch_discard = true; return 0;\n'
             '        }\n'
             '        if (twatch_touch_discard) {\n'
             '            if (tp != 0) return 0; // suppress the interrupted gesture\n'
             '            twatch_touch_discard = false;\n'
             '        }')
replace_once(ultra,
             '    if (devices_probe & HW_DRV_ONLINE) {\n        drv.setWaveform(0, _effects);',
             '    if ((devices_probe & HW_DRV_ONLINE) && twatch_i2c_ready(0x5A)) {\n        drv.setWaveform(0, _effects);')
replace_once(ultra,
             '    bool batteryInsert = pmu.isBatteryConnect();\n    // Get PMU Interrupt Status Register\n    pmu.getIrqStatus();',
             '    const auto before = twatch_i2c_failures();\n'
             '    bool batteryInsert = pmu.isBatteryConnect();\n'
             '    // Get PMU Interrupt Status Register\n    pmu.getIrqStatus();\n'
             '    if (twatch_i2c_failures() != before) {\n'
             '        xEventGroupSetBits(_event, HW_IRQ_POWER);\n'
             '        return; // neither dispatch stale IRQ nor clear unread PMU registers\n    }')
replace_once(ultra,
             '    // Clear PMU Interrupt Status Register\n    pmu.clearIrqStatus();',
             '    // Clear PMU Interrupt Status Register\n    pmu.clearIrqStatus();\n'
             '    if (twatch_i2c_failures() != before) xEventGroupSetBits(_event, HW_IRQ_POWER);')
# The later Phase 9 patch extends this same anchor; skip if already applied.
if '((bits & HW_IRQ_POWER) || digitalRead(PMU_INT) == LOW)' not in open(ultra, encoding='utf-8').read():
    replace_once(ultra,
             '    if (bits & HW_IRQ_POWER) {\n        clearEventBits(HW_IRQ_POWER);\n        checkPowerStatus();\n    }',
             '    if ((bits & HW_IRQ_POWER) && twatch_i2c_ready(0x34)) {\n'
             '        clearEventBits(HW_IRQ_POWER);\n        checkPowerStatus();\n    }')
# Retentive sleep: an IRQ that went low while the CPU slept may not produce
# another falling edge after wake. Service a latched low PMU level too.
replace_once(ultra,
             '    if ((bits & HW_IRQ_POWER) && twatch_i2c_ready(0x34)) {',
             '    if (((bits & HW_IRQ_POWER) || digitalRead(PMU_INT) == LOW) && twatch_i2c_ready(0x34)) {')
replace_once(ultra,
             '    if (bits & HW_IRQ_SENSOR) {\n        clearEventBits(HW_IRQ_SENSOR);\n        sensor.update();\n        sendEvent(SENSOR_EVENT);\n    }',
             '    if ((bits & HW_IRQ_SENSOR) && twatch_i2c_ready(0x28)) {\n'
             '        clearEventBits(HW_IRQ_SENSOR);\n'
             '        const auto before = twatch_i2c_failures();\n'
             '        sensor.update();\n'
             '        if (twatch_i2c_failures() != before) xEventGroupSetBits(_event, HW_IRQ_SENSOR);\n'
             '        else sendEvent(SENSOR_EVENT);\n    }')
# The SDK normally uses 400 kHz after booting the BHI260; retain this on wake.
replace_once(ultra, '    Wire.begin(SDA, SCL);\n\n    radio.standby();',
             '    Wire.begin(SDA, SCL);\n    Wire.setClock(400000UL);\n\n    radio.standby();')

# Never interpret an expander communication failure as a card insertion.
replace_once(ultra,
             '    io.pinMode(EXPANDS_SD_DET, INPUT);\n    if (io.digitalRead(EXPANDS_SD_DET) == HIGH) {',
             '    const auto before = twatch_i2c_failures();\n'
             '    io.pinMode(EXPANDS_SD_DET, INPUT);\n'
             '    const auto detect = io.digitalRead(EXPANDS_SD_DET);\n'
             '    if (twatch_i2c_failures() != before || detect == HIGH) {')

# Cancel an interrupted LVGL gesture instead of generating a false click or
# extending a stale press while the touch controller is unavailable.
replace_once(os.path.join(src_dir, "LV_Helper_v9.cpp"),
             '#ifdef USING_INPUT_DEV_TOUCHPAD\nstatic void touchpad_read',
             '#ifdef USING_INPUT_DEV_TOUCHPAD\n#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             'extern "C" bool twatch_i2c_touch_failed();\n#endif\nstatic void touchpad_read')
replace_once(os.path.join(src_dir, "LV_Helper_v9.cpp"),
             '    uint8_t touched = plane->getPoint(&x, &y, 1);',
             '    uint8_t touched = plane->getPoint(&x, &y, 1);\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    if (twatch_i2c_touch_failed()) {\n'
             '        lv_indev_reset(drv, nullptr);\n'
             '        data->state = LV_INDEV_STATE_REL;\n'
             '        return;\n    }\n#endif')

# Phase 6: keep the stock boot sequence, but route runtime Ultra QSPI writes
# through our persistent asynchronous DMA pipeline. No other board is changed.
qspi = os.path.join(src_dir, "LilyGoDispInterface.cpp")
replace_once(qspi, '#include <bsp_lcd/esp_lcd_st7796.h>',
             '#include <bsp_lcd/esp_lcd_st7796.h>\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             'extern "C" void twatch_qspi_bind(spi_device_handle_t,int,uint16_t,uint16_t);\n'
             'extern "C" void twatch_qspi_offset(uint16_t,uint16_t);\n'
             'extern "C" bool twatch_qspi_enabled();\n'
             'extern "C" bool twatch_qspi_command(uint32_t,uint8_t*,uint32_t);\n'
             'extern "C" bool twatch_qspi_end();\n'
             'extern "C" void twatch_qspi_push_sync(uint16_t,uint16_t,uint16_t,uint16_t,uint16_t*);\n'
             '#endif')
replace_once(qspi,
             '    pushColors(0, 0, width, height, draw_buf.data());\n\n    return true;',
             '    pushColors(0, 0, width, height, draw_buf.data());\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    twatch_qspi_bind(_spi_dev,_cs,_offset_x,_offset_y);\n#endif\n\n    return true;')
replace_once(qspi,
             '    writeCommand(DISP_CMD_MADCTL, &gbr, 1);',
             '    writeCommand(DISP_CMD_MADCTL, &gbr, 1);\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    twatch_qspi_offset(_offset_x,_offset_y);\n#endif')
replace_once(qspi,
             'void LilyGoDispQSPI::pushColors(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color)\n{',
             'void LilyGoDispQSPI::pushColors(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2, uint16_t *color)\n{\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    if (twatch_qspi_enabled()) { twatch_qspi_push_sync(x1,y1,x2,y2,color); return; }\n#endif')
replace_once(qspi,
             'void LilyGoDispQSPI::writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length)\n{',
             'void LilyGoDispQSPI::writeCommand(uint32_t cmd, uint8_t *pdat, uint32_t length)\n{\n'
             '    if (!_spi_dev) return; // constructor rotation occurs before bus is initialized\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    if (twatch_qspi_enabled()) { twatch_qspi_command(cmd,pdat,length); return; }\n#endif')
replace_once(qspi,
             'void LilyGoDispQSPI::end()\n{',
             'void LilyGoDispQSPI::end()\n{\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    if (twatch_qspi_enabled() && !twatch_qspi_end()) return; // never free an active descriptor\n#endif')
replace_once(qspi,
             '    _brightness = level;\n    writeCommand(DISP_CMD_BRIGHTNESS, &level, 1);',
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA)\n'
             '    if (twatch_qspi_enabled()) {\n'
             '        if (twatch_qspi_command(DISP_CMD_BRIGHTNESS,&level,1)) _brightness=level;\n'
             '        return;\n    }\n#endif\n'
             '    _brightness = level;\n    writeCommand(DISP_CMD_BRIGHTNESS, &level, 1);')
# PSRAM rendering is double-buffered but partial; separate static DMA staging
# is allocated in firmware. The second buffer is optional under heap pressure.
replace_once(os.path.join(src_dir, "LV_Helper_v9.cpp"),
             '        log_d("Using Not DMA pushColors..");\n        buf = (lv_color16_t *)ps_malloc(lv_buffer_size);\n        assert(buf);\n        buf1 = (lv_color16_t *)ps_malloc(lv_buffer_size);\n        assert(buf1);',
             '        log_d("Using Not DMA pushColors..");\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA) && TWATCH_QSPI_ASYNC\n'
             '        lv_buffer_size = 8192; // matches the dedicated DMA staging capacity\n'
             '#endif\n'
             '        buf = (lv_color16_t *)ps_malloc(lv_buffer_size);\n        assert(buf);\n'
             '        buf1 = (lv_color16_t *)ps_malloc(lv_buffer_size);\n'
             '#if !defined(ARDUINO_T_WATCH_S3_ULTRA) || !TWATCH_QSPI_ASYNC\n        assert(buf1);\n#endif')

replace_once(os.path.join(src_dir, "LV_Helper_v9.cpp"),
             '    bool useDMA = board.useDMA();',
             '    bool useDMA = board.useDMA();\n'
             '#if defined(ARDUINO_T_WATCH_S3_ULTRA) && TWATCH_QSPI_ASYNC\n'
             '    useDMA = false; // LVGL buffers are PSRAM; firmware owns DMA staging\n#endif')
