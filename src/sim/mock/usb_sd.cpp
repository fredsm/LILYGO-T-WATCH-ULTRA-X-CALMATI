#include "../../usb_sd.h"
namespace { bool mounted=false; }
void usb_sd_init(){mounted=false;}
bool usb_sd_start(){mounted=true;return true;}
void usb_sd_stop(){mounted=false;}
bool usb_sd_is_running(){return mounted;}
bool usb_sd_host_active(){return false;}
uint64_t usb_sd_card_bytes(){return 32000000000ULL;}
