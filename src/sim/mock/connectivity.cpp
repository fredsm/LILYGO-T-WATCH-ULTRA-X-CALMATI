#include "../../mouse_hid.h"
#include "../../webpanel.h"
namespace { bool mouse=false,web=false; }
bool mouse_hid_start(){mouse=true;return true;}
void mouse_hid_stop(){mouse=false;}
bool mouse_hid_is_running(){return mouse;}
bool mouse_hid_is_connected(){return mouse;}
void mouse_hid_move(int,int){}
void mouse_hid_click(uint8_t){}
void mouse_hid_scroll(int){}
void webpanel_start(){web=true;}
void webpanel_stop(){web=false;}
bool webpanel_is_running(){return web;}
void webpanel_tick(){}
const char* webpanel_ssid(){return "TWatch-SIM";}
