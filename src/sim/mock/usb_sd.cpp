#include "../../usb_sd.h"
#include "../../storage_manager.h"
#include <lvgl.h>
void usb_sd_init(){}
bool usb_sd_start(){if(storage_usb_owned())return true;storage_release_image_cache();return storage_try_usb_claim();}
void usb_sd_stop(){storage_usb_release();if(storage_needs_remount()&&storage_idle())storage_media_inserted();}
bool usb_sd_is_running(){return storage_usb_owned();}
bool usb_sd_host_active(){return false;}
uint64_t usb_sd_card_bytes(){return storage_stats().media?32000000000ULL:0;}
