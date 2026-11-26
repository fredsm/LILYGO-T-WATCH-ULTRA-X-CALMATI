#include "startup.hpp"
#include "clock.hpp"
#include "screens.hpp"
#include "../onboarding.h"
namespace {
lv_obj_t* splash=nullptr;
lv_obj_t* onboard=nullptr;
uint32_t started=0;
bool requested=false,created=false;
}
namespace sim {
void start_boot(bool onboarding){
    if(!created){onboarding_screen_create();created=true;}
    if(splash)return;
    requested=onboarding;started=lv_tick_get();
    splash=create_boot_splash();
    // Only the transition is host-specific; the layout/art are original firmware.
    lv_obj_set_style_opa(splash,LV_OPA_TRANSP,0);
}
void tick_boot(){
    if(!splash)return;
    auto elapsed=lv_tick_get()-started;
    lv_obj_set_style_opa(splash,elapsed>=300?LV_OPA_COVER:elapsed*255/300,0);
    if(elapsed<1500)return;
    if(requested&&onboarding_needed()){onboarding_screen_show();onboard=lv_screen_active();}
    else show_home();
    lv_obj_delete(splash);splash=nullptr;
}
bool startup_active(){return splash||(onboard&&lv_screen_active()==onboard);}
}
