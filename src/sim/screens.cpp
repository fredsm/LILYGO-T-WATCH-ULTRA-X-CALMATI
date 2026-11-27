#include "../camera_screen.h"
#include "../gatt_explorer_screen.h"
#include "../map_screen.h"
#include "../nfc_screen.h"
#include "../nfc_write_screen.h"
#include "../nfc_credit_screen.h"
#include "../pet_screen.h"
#include "../skimmer_screen.h"
#include "../voice_screen.h"
#include "../wardriver_screen.h"
#include "../arp_mitm_screen.h"
#include "../badusb_screen.h"
#include "../deauther_screen.h"
#include "../wpa3_sae_screen.h"
#include "../evil_twin_screen.h"
#include "../drone_screen.h"
#include "../rid_spoof_screen.h"
#include "../phantom_flood_screen.h"
#include "../mifare_screen.h"
#include "../scan_screen.h"
#include "screens.hpp"
#include "clock.hpp"
#include "navigation.hpp"
#include "../layout_config.h"
#include "startup.hpp"
#include "../pin_lock_screen.h"
#include "../tools_screen.h"
#include "../task_manager.h"
#include "../diagnostics_manager.h"
#include "../event_manager.h"
#include "../diagnostics_screen.h"
#include <ctime>
#include "../stopwatch_screen.h"
#include "../calendar_screen.h"
#include "../timer_screen.h"
#include "../alarm_screen.h"
#include "../time_screen.h"
#include "../usb_sd_screen.h"
#include "../storage_manager.h"
#include "mock/hardware.hpp"
#include "../mouse_screen.h"
#include "../webpanel_screen.h"
#include "../pager_screen.h"
#include "../tpms_screen.h"
#include "../aprs_screen.h"
#include "../adsb_screen.h"
#include "../mic_screen.h"
#include "../gps_screen.h"
#include "../car_screen.h"
#include "../nodes_screen.h"
#include "../configuration_screen.h"
#include "../send_message_screen.h"
#include "../channels_screen.h"
#include "../meshtastic_screen.h"
#include "../espnow_screen.h"
#include "../lora_screen.h"
#include "../wifi_radio_screen.h"
#include "../calmati_remote_screen.h"
#include "../calmati_targets_screen.h"
#include "../calmati_result_screen.h"
#include "../settings_screen.h"
#include "../wifi_screen.h"
#include "../bluetooth_screen.h"
#include "../meta_screen.h"
#include "../printer_screen.h"
#include "../analyze_screen.h"
#include "../bt_analyze_screen.h"
#include "../lora_analyze_screen.h"
#include "../portscan_screen.h"
#include "../ble_pair_screen.h"
#include "../waterfall_screen.h"
#include "../subghz_sentinel_screen.h"
#include "../threat_radar_screen.h"
#include "../tesla_cp_screen.h"

namespace {
lv_obj_t* home_screen=nullptr;
lv_obj_t* voice_screen=nullptr;
void show_voice(){voice_screen_show();voice_screen=lv_screen_active();}
bool voice_active(){return voice_screen && lv_screen_active()==voice_screen;}
void show_results(){calmati_result_screen_show("SIM RESULTS","/demo",false);}
void show_ports(){portscan_screen_show(0xc000020a,"SIM host");}
lv_obj_t* task_screen=nullptr;
void show_task(){task_manager_show();task_screen=lv_screen_active();}
bool task_active(){return task_screen && lv_screen_active()==task_screen;}
const sim::Screen registry[]={
#define SIM_SCREEN(name,create,show,active,stop) {#name,create,show,active,stop},
#include "screens.inc"
#undef SIM_SCREEN
 {"task_manager",nullptr,show_task,task_active,nullptr},
 {"diagnostics",nullptr,diagnostics_screen_show,diagnostics_screen_is_active,nullptr},
 {"quick_controls",nullptr,sim::show_quick_controls,sim::quick_controls_active,nullptr},
};
}
namespace sim {
void initialize_filesystem();
const Screen* screens(std::size_t& count){count=sizeof(registry)/sizeof(registry[0]);return registry;}
void initialize_screens(lv_obj_t* home){event_manager_init();ui_task_claim();create_clock();home_screen=home;initialize_filesystem();storage_init(sim::hardware.sd_ready);storage_bind_lvgl();layout_load();for(const auto& s:registry)if(s.create)s.create();initialize_navigation();lv_screen_load(home);diagnostics_init();}
void tick(){refresh_clock();nfc_screen_worker();nfc_write_screen_worker();nfc_credit_screen_worker();mifare_screen_worker();wardriver_bg_tick();wardriver_screen_update();if(!sim::startup_active())pin_lock_enforce();}
void leave_screen(){for(const auto& s:registry)if(s.active()&&s.stop)s.stop();}
void show_home(){leave_screen();lv_screen_load(home_screen);}
}
void clock_screen_show(){sim::show_home();}
