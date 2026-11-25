#pragma once
constexpr int RESTART_BOOTLOADER=0;
inline bool sim_bootloader_requested=false;
inline void usb_persist_restart(int){sim_bootloader_requested=true;}
