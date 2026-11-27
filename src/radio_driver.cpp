// Linker wrappers around shared controller lifecycle calls, not packet handling.
#ifndef TWATCH_SIMULATOR
#include "radio_manager.h"
#include <esp_wifi.h>
#include <esp_bt.h>
#include <esp_bt_main.h>
#include <esp_timer.h>
namespace {
template<class F> esp_err_t guarded(RadioAction action,F call){
    RadioTransition lock;
    if(!lock)return ESP_ERR_INVALID_STATE;
    wifi_mode_t mode;
    bool wifi=action==RadioAction::BleUp && esp_wifi_get_mode(&mode)==ESP_OK;
    bool ble=action==RadioAction::WifiUp && esp_bt_controller_get_status()!=ESP_BT_CONTROLLER_STATUS_IDLE;
    if(!radio_action_allowed(action,wifi,ble))return ESP_ERR_INVALID_STATE;
    auto start=esp_timer_get_time();
    auto result=call();
    radio_action_record(action,result==ESP_OK,esp_timer_get_time()-start);
    return result;
}
}
#define WRAP0(name, action) \
extern "C" esp_err_t __real_##name(void); \
extern "C" esp_err_t __wrap_##name(void){ \
    return guarded(RadioAction::action,[]{return __real_##name();}); \
}
#define WRAP1(name, action, arg_type) \
extern "C" esp_err_t __real_##name(arg_type); \
extern "C" esp_err_t __wrap_##name(arg_type arg){ \
    return guarded(RadioAction::action,[&]{return __real_##name(arg);}); \
}
WRAP1(esp_wifi_init,WifiUp,const wifi_init_config_t*)
WRAP0(esp_wifi_start,WifiUp)
WRAP0(esp_wifi_stop,WifiDown)
WRAP0(esp_wifi_deinit,WifiDown)
extern "C" esp_err_t __real_esp_wifi_set_mode(wifi_mode_t);
extern "C" esp_err_t __wrap_esp_wifi_set_mode(wifi_mode_t mode){
    return guarded(mode==WIFI_MODE_NULL?RadioAction::WifiDown:RadioAction::WifiUp,
        [&]{return __real_esp_wifi_set_mode(mode);});
}
WRAP0(esp_wifi_connect,WifiUp)
WRAP0(esp_wifi_disconnect,WifiDown)
WRAP1(esp_bt_controller_init,BleUp,esp_bt_controller_config_t*)
WRAP1(esp_bt_controller_enable,BleUp,esp_bt_mode_t)
WRAP0(esp_bt_controller_disable,BleDown)
WRAP0(esp_bt_controller_deinit,BleDown)
WRAP0(esp_bluedroid_init,BleUp)
WRAP0(esp_bluedroid_enable,BleUp)
WRAP0(esp_bluedroid_disable,BleDown)
WRAP0(esp_bluedroid_deinit,BleDown)
extern "C" esp_err_t __real_esp_wifi_set_config(wifi_interface_t,wifi_config_t*);
extern "C" esp_err_t __wrap_esp_wifi_set_config(wifi_interface_t iface,wifi_config_t* cfg){
    return guarded(RadioAction::WifiChange,[&]{return __real_esp_wifi_set_config(iface,cfg);});
}
#endif
