#pragma once
#include <stddef.h>
#include <stdint.h>

// Boot-time presentation only. No code, radio actions or executable paths.
constexpr const char* LAYOUT_PATH="/Settings/layout.txt";
constexpr const char* LAYOUT_DEFAULT_PATH="/Settings/layout_default.txt";
constexpr size_t LAYOUT_MAX_BYTES=4096;
enum class HomeInput:uint8_t { SwipeLeft, SwipeRight, SwipeUp, SwipeDown, Back, Power, Count };
enum class LayoutSource:uint8_t { Embedded, DefaultFile, CustomFile };
struct LayoutStatus { LayoutSource source; bool invalid_default, invalid_custom; };
enum class HomeTarget:uint8_t {
    None, Time, Meshtastic, Tools, QuickControls, Settings, Gps, LoRa,
    WifiRadio, Bluetooth, Nfc, Calendar, Stopwatch, Timer, Alarm, Diagnostics, TaskManager
};
constexpr unsigned LAYOUT_TOOL_COUNT=44;
constexpr unsigned LAYOUT_CONTROL_COUNT=6;
constexpr unsigned LAYOUT_ACTION_COUNT=2;
struct LayoutConfig {
    HomeTarget home[static_cast<unsigned>(HomeInput::Count)];
    uint8_t tools[LAYOUT_TOOL_COUNT], tool_count;
    bool hidden[LAYOUT_TOOL_COUNT];
    uint8_t controls[LAYOUT_CONTROL_COUNT], control_count;
    uint8_t actions[LAYOUT_ACTION_COUNT], action_count;
};
LayoutConfig layout_defaults();
// Atomic parse: invalid file leaves output unchanged. Input is bounded and writable.
bool layout_parse(char* text,size_t length,LayoutConfig& output);
// Overlay a valid file on a previously validated base; otherwise leave output alone.
bool layout_parse_overlay(char* text,size_t length,const LayoutConfig& base,LayoutConfig& output);
bool layout_embedded_valid();
const char* layout_tool_name(unsigned id);
const LayoutConfig& layout_config();
LayoutStatus layout_status();
void layout_load(); // once after SD storage initialization, before screen creation
void layout_open_home(HomeInput input);
