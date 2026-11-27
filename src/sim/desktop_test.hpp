#pragma once
#include "panel.hpp"
#include "../power_manager.h"
#include "clock.hpp"
#include "navigation.hpp"
#include "../gps_screen.h"
#include "../wardriver_screen.h"
#include "../meshtastic_screen.h"
#include "../tools_screen.h"
#include "../time_screen.h"
#include "../settings_screen.h"
#include "../send_message_screen.h"
#include "keyboard.hpp"
#include "mock/LilyGoLib.h"
#include <functional>
#include <vector>
#include <cstdio>
namespace sim {
// Drives the same SDL event queue as real mouse/keyboard input, in CI only.
class DesktopTest {
    std::vector<std::function<void()>> steps;
    size_t position=0;
    Uint32 watch_id;
    HardwarePanel& panel;
    static lv_obj_t* find(lv_obj_t* root,const lv_obj_class_t* type,const char* text=nullptr){
        if(lv_obj_check_type(root,type)&&(!text||!std::strcmp(lv_label_get_text(root),text)))return root;
        for(unsigned i=0;i<lv_obj_get_child_count(root);++i)if(auto* r=find(lv_obj_get_child(root,i),type,text))return r;
        return nullptr;
    }
    void wait(int n=3){while(n--)steps.push_back([]{});}
    void check(std::function<bool()> condition,const char* name){steps.push_back([this,condition,name]{bool pass=condition();std::printf("SDL %s: %s\n",name,pass?"PASS":"FAIL");std::fflush(stdout);ok&=pass;});}
    void key(SDL_Keycode code,Uint32 id){for(auto type:{SDL_KEYDOWN,SDL_KEYUP})steps.push_back([=]{SDL_Event e{};e.type=type;e.key.windowID=id;e.key.keysym.sym=code;SDL_PushEvent(&e);});wait();}
    void sample(Uint32 id,int x,int y,int type){steps.push_back([=]{SDL_Event e{};e.type=type;if(type==SDL_MOUSEMOTION){e.motion.windowID=id;e.motion.x=x;e.motion.y=y;}else{e.button.windowID=id;e.button.button=SDL_BUTTON_LEFT;e.button.x=x;e.button.y=y;}SDL_PushEvent(&e);});}
    void swipe(int x,int y,int dx,int dy){sample(watch_id,x,y,SDL_MOUSEBUTTONDOWN);for(int i=1;i<=8;++i)sample(watch_id,x+dx*i/8,y+dy*i/8,SDL_MOUSEMOTION);sample(watch_id,x+dx,y+dy,SDL_MOUSEBUTTONUP);wait(16);}
    void tap_panel(lv_obj_t* object){
        for(auto type:{SDL_MOUSEBUTTONDOWN,SDL_MOUSEBUTTONUP})steps.push_back([this,object,type]{lv_obj_update_layout(object);lv_area_t a;lv_obj_get_coords(object,&a);SDL_Event e{};e.type=type;e.button.windowID=panel.id();e.button.button=SDL_BUTTON_LEFT;e.button.x=a.x1+8;e.button.y=(a.y1+a.y2)/2;SDL_PushEvent(&e);});
        wait();
    }
public:
    bool ok=true;
    DesktopTest(SDL_Window* watch,HardwarePanel& p):watch_id(SDL_GetWindowID(watch)),panel(p){
        auto home=[](){return lv_screen_active()==create_clock();};
        check(home,"starts on original clock");check([]{return hardware.watch.brightness()==255;},"full brightness default");
        key(SDLK_F2,watch_id);check(gps_screen_is_active,"F2 opens GPS");key(SDLK_F4,watch_id);check(home,"F4 returns home");
        swipe(330,250,-240,0);check(time_screen_is_active,"left opens Time");swipe(25,12,300,0);check(home,"Time returns home");
        swipe(80,250,240,0);check(meshtastic_screen_is_active,"right opens mesh");swipe(350,12,-300,0);check(home,"mesh returns home");
        swipe(205,100,0,240);check(quick_controls_active,"down opens quick controls");
        sample(watch_id,205,425,SDL_MOUSEBUTTONDOWN);sample(watch_id,205,425,SDL_MOUSEBUTTONUP);wait();
        check(settings_screen_is_active,"quick Settings shortcut opens settings");key(SDLK_F4,watch_id);
        swipe(205,100,0,240);swipe(5,400,0,-300);check(home,"quick controls returns home");
        swipe(205,330,0,-240);check(tools_screen_is_active,"up opens tools");
        steps.push_back([this]{auto* label=find(lv_screen_active(),&lv_label_class,"Wardriver");if(!label){ok=false;return;}lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,nullptr);});wait();
        check(wardriver_screen_is_active,"Tools opens Wardriver");swipe(25,12,300,0);check(tools_screen_is_active,"Wardriver returns to Tools");
        steps.push_back([this]{auto* label=find(lv_screen_active(),&lv_label_class,"Time");if(!label){ok=false;return;}auto* tile=lv_obj_get_parent(label);if(lv_obj_get_index(tile)!=0){ok=false;return;}lv_obj_send_event(tile,LV_EVENT_CLICKED,nullptr);});wait();
        check(time_screen_is_active,"first Tools tile opens Time");swipe(5,80,0,300);check(tools_screen_is_active,"Time returns to Tools");
        swipe(5,80,0,300);check(home,"tools edge returns home");
        tap_panel(panel.battery);wait(60);
        check([]{char text[16];std::snprintf(text,sizeof(text),"%d%%",instance.pmu.getBatteryPercent());return instance.pmu.getBatteryPercent()<10&&find(create_clock(),&lv_label_class,text);},"panel battery updates original clock");
        tap_panel(panel.brightness);check([]{return hardware.watch.brightness()<20;},"panel brightness reaches renderer state");
        steps.push_back([]{instance.setBrightness(255);});wait();
        check([this]{return lv_slider_get_value(panel.brightness)==255;},"firmware brightness synchronizes panel");
        tap_panel(panel.charging);check([]{return instance.pmu.isCharging();},"panel charging reaches PMU");
        tap_panel(panel.gps);check([]{return !instance.gps.location.isValid();},"panel GPS loss reaches firmware");
        tap_panel(panel.sd);check([]{return !instance.isCardReady();},"panel SD removal reaches firmware");
        key(SDLK_F5,watch_id);check([]{return !hardware.watch.display_on();},"watch display off");
        sample(watch_id,90,530,SDL_MOUSEBUTTONDOWN);sample(watch_id,90,530,SDL_MOUSEBUTTONUP);wait();check(gps_screen_is_active,"physical Power works with display off");
        tap_panel(panel.display_on);check([]{return hardware.watch.display_on();},"panel wakes watch");
        key(SDLK_F2,panel.id());check(gps_screen_is_active,"panel keyboard isolated from watch");
        steps.push_back([this]{SDL_Event e{};e.type=SDL_WINDOWEVENT;e.window.windowID=panel.id();e.window.event=SDL_WINDOWEVENT_CLOSE;SDL_PushEvent(&e);});wait();
        check([this]{return (SDL_GetWindowFlags(panel.window)&SDL_WINDOW_HIDDEN)!=0;},"panel close hides only panel");
        key(SDLK_F6,watch_id);check([this]{return (SDL_GetWindowFlags(panel.window)&SDL_WINDOW_SHOWN)!=0;},"F6 reopens panel");
        key(SDLK_F4,watch_id);check(home,"home after panel interaction");
        key(SDLK_F1,watch_id);check([]{return find(lv_screen_active(),&lv_label_class,"T-Watch Ultra | SIMULATED DATA")!=nullptr;},"F1 opens screen catalog");
        key(SDLK_F4,watch_id);
        for(auto type:{SDL_FINGERDOWN,SDL_FINGERUP})steps.push_back([this,type]{SDL_Event e{};e.type=type;e.tfinger.windowID=watch_id;e.tfinger.fingerId=7;e.tfinger.x=90.0f/409;e.tfinger.y=530.0f/561;SDL_PushEvent(&e);});
        wait();check(gps_screen_is_active,"native finger taps physical Power");key(SDLK_F4,watch_id);
        steps.push_back([this]{send_message_screen_show();auto* label=find(lv_screen_active(),&lv_label_class,"+ Compose...");if(!label){ok=false;return;}lv_obj_send_event(lv_obj_get_parent(label),LV_EVENT_CLICKED,nullptr);});wait();
        check(text_input_active,"original compose editor active");
        steps.push_back([this]{SDL_Event e{};e.type=SDL_TEXTINPUT;e.text.windowID=watch_id;std::strcpy(e.text.text,"pbM cafe");SDL_PushEvent(&e);});wait();
        check([]{auto* a=find(lv_screen_active(),&lv_textarea_class);return a&&!std::strcmp(lv_textarea_get_text(a),"pbM cafe");},"SDL text reaches firmware without shortcuts");
        key(SDLK_BACKSPACE,watch_id);
        check([]{auto* a=find(lv_screen_active(),&lv_textarea_class);return a&&!std::strcmp(lv_textarea_get_text(a),"pbM caf");},"SDL Backspace edits text");
        key(SDLK_ESCAPE,watch_id);check([]{return !text_input_active()&&send_message_screen_is_active();},"Escape cancels editor without quitting");
        key(SDLK_F4,watch_id);check(home,"final return to clock");
        // Real SDL mouse hold with deterministic 20 ms frames.
        sample(watch_id,90,530,SDL_MOUSEBUTTONDOWN);wait(240);
        check([]{return !power_screen_off();},"less than five seconds stays on");
        wait(15);check([]{return power_screen_locked()&&power_screen_off()&&!power_is_asleep();},"five seconds locks and blanks before release");
        sample(watch_id,90,530,SDL_MOUSEBUTTONUP);wait();
        check(power_is_asleep,"release enters deep sleep");
        key(SDLK_F3,watch_id);key(SDLK_F4,watch_id);key(SDLK_F5,watch_id);
        swipe(205,330,0,-240);tap_panel(panel.display_on);
        check(power_is_asleep,"back, touch and panel cannot wake power-off");
        key(SDLK_F2,watch_id);wait(85);
        check([home]{return !power_is_asleep()&&!power_screen_locked()&&!power_screen_off()&&home();},"power button alone reboots to clock");
    }
    bool tick(){if(position==steps.size()||!ok)return false;steps[position++]();return true;}
    bool complete()const{return ok&&position==steps.size();}
};
}
