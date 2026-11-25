#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <lvgl.h>
#include "mock/watch.hpp"
#include "../stopwatch_screen.h"
#include "../calendar_screen.h"
#include "../timer_screen.h"
#include "../alarm_screen.h"
#include "../time_screen.h"
#include "../usb_sd_screen.h"
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
#include <ctime>
void clock_screen_get_local_time(struct tm* out) { auto t=std::time(nullptr); *out=*std::localtime(&t); }
static lv_obj_t* preview_screen=nullptr;
void clock_screen_show() { lv_screen_load(preview_screen); }
void tools_screen_show() { lv_screen_load(preview_screen); }
bool clock_screen_uses_12h() { return false; }
#include <array>
#include <cstdint>
#include <cstdio>
namespace {
constexpr int W=502,H=410;
std::array<uint16_t,W*H> pixels{};
std::array<uint16_t,W*32> buffer{};
SDL_Texture* texture=nullptr;
SDL_Renderer* renderer=nullptr;
sim::Watch watch;
lv_point_t pointer{};
bool pressed=false;
lv_obj_t* status=nullptr;
void flush(lv_display_t* d,const lv_area_t* a,uint8_t* data) {
    auto* src=reinterpret_cast<const uint16_t*>(data);
    const int width=a->x2-a->x1+1;
    for(int y=a->y1;y<=a->y2;++y)
        for(int x=a->x1;x<=a->x2;++x)
            if(x>=0 && x<W && y>=0 && y<H)
                pixels[y*W+x]=src[(y-a->y1)*width+x-a->x1];
    lv_display_flush_ready(d);
}
void read_pointer(lv_indev_t*,lv_indev_data_t* d) {
    d->point=pointer;
    d->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
}
void refresh() {
    lv_label_set_text_fmt(status,"Battery %d%% | Brightness %d\n%s | %s",watch.battery(),watch.brightness(),watch.charging()?"Charging":"Unplugged",watch.motion()?"Motion":"Stationary");
}
void battery_changed(lv_event_t* e) {
    watch.set_battery(lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e))));
    refresh();
}
void brightness_changed(lv_event_t* e) {
    watch.set_brightness(lv_slider_get_value(static_cast<lv_obj_t*>(lv_event_get_target(e))));
    refresh();
}
void charging_changed(lv_event_t* e) {
    watch.set_charging(lv_obj_has_state(static_cast<lv_obj_t*>(lv_event_get_target(e)),LV_STATE_CHECKED));
    refresh();
}
void preview() {
    auto* screen=lv_screen_active();
    preview_screen=screen;
    lv_obj_set_flex_flow(screen,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    auto* title=lv_label_create(screen);
    lv_label_set_text(title,"T-Watch Ultra | UI runtime preview");
    status=lv_label_create(screen);
    refresh();
    auto* battery=lv_slider_create(screen);
    lv_obj_set_width(battery,300);
    lv_slider_set_range(battery,0,100);
    lv_slider_set_value(battery,watch.battery(),LV_ANIM_OFF);
    lv_obj_add_event_cb(battery,battery_changed,LV_EVENT_VALUE_CHANGED,nullptr);
    auto* brightness=lv_slider_create(screen);
    lv_obj_set_width(brightness,300);
    lv_slider_set_range(brightness,0,255);
    lv_slider_set_value(brightness,watch.brightness(),LV_ANIM_OFF);
    lv_obj_add_event_cb(brightness,brightness_changed,LV_EVENT_VALUE_CHANGED,nullptr);
    auto* charge=lv_checkbox_create(screen);
    lv_checkbox_set_text(charge,"Charging");
    lv_obj_add_event_cb(charge,charging_changed,LV_EVENT_VALUE_CHANGED,nullptr);
    auto* help=lv_label_create(screen);
    lv_label_set_text(help,"P: power | B: back | M: motion");
    auto* btn=lv_button_create(screen);
    auto* lbl=lv_label_create(btn);
    lv_label_set_text(lbl,"Original stopwatch");
    lv_obj_add_event_cb(btn,[](lv_event_t*){ stopwatch_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* cal=lv_button_create(screen);
    lv_label_set_text(lv_label_create(cal),"Original calendar");
    lv_obj_add_event_cb(cal,[](lv_event_t*){ calendar_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* time=lv_button_create(screen);
    lv_label_set_text(lv_label_create(time),"Original time menu");
    lv_obj_add_event_cb(time,[](lv_event_t*){ time_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* usb=lv_button_create(screen);
    lv_label_set_text(lv_label_create(usb),"Original USB SD");
    lv_obj_add_event_cb(usb,[](lv_event_t*){ usb_sd_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* mouse=lv_button_create(screen);
    lv_label_set_text(lv_label_create(mouse),"Original mouse");
    lv_obj_add_event_cb(mouse,[](lv_event_t*){ mouse_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* web=lv_button_create(screen);
    lv_label_set_text(lv_label_create(web),"Original web panel");
    lv_obj_add_event_cb(web,[](lv_event_t*){ webpanel_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_pager=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_pager),"Original pager");
    lv_obj_add_event_cb(b_pager,[](lv_event_t*){ pager_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_tpms=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_tpms),"Original tpms");
    lv_obj_add_event_cb(b_tpms,[](lv_event_t*){ tpms_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_aprs=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_aprs),"Original aprs");
    lv_obj_add_event_cb(b_aprs,[](lv_event_t*){ aprs_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_adsb=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_adsb),"Original adsb");
    lv_obj_add_event_cb(b_adsb,[](lv_event_t*){ adsb_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_mic=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_mic),"Original mic");
    lv_obj_add_event_cb(b_mic,[](lv_event_t*){ mic_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_gps=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_gps),"Original gps");
    lv_obj_add_event_cb(b_gps,[](lv_event_t*){ gps_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_car=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_car),"Original car");
    lv_obj_add_event_cb(b_car,[](lv_event_t*){ car_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_nodes=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_nodes),"Original nodes");
    lv_obj_add_event_cb(b_nodes,[](lv_event_t*){ nodes_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_configuration=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_configuration),"Original configuration");
    lv_obj_add_event_cb(b_configuration,[](lv_event_t*){ configuration_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_send_message=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_send_message),"Original send_message");
    lv_obj_add_event_cb(b_send_message,[](lv_event_t*){ send_message_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_channels=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_channels),"Original channels");
    lv_obj_add_event_cb(b_channels,[](lv_event_t*){ channels_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_meshtastic=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_meshtastic),"Original meshtastic");
    lv_obj_add_event_cb(b_meshtastic,[](lv_event_t*){ meshtastic_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_espnow=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_espnow),"Original espnow");
    lv_obj_add_event_cb(b_espnow,[](lv_event_t*){ espnow_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_lora=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_lora),"Original lora");
    lv_obj_add_event_cb(b_lora,[](lv_event_t*){ lora_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_wifi_radio=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_wifi_radio),"Original wifi_radio");
    lv_obj_add_event_cb(b_wifi_radio,[](lv_event_t*){ wifi_radio_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_calmati_remote=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_calmati_remote),"Original calmati_remote");
    lv_obj_add_event_cb(b_calmati_remote,[](lv_event_t*){ calmati_remote_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_calmati_targets=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_calmati_targets),"Original calmati_targets");
    lv_obj_add_event_cb(b_calmati_targets,[](lv_event_t*){ calmati_targets_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_calmati_result=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_calmati_result),"Original calmati_result");
    lv_obj_add_event_cb(b_calmati_result,[](lv_event_t*){ calmati_result_screen_show("SIM RESULTS","/demo",false); },LV_EVENT_CLICKED,nullptr);
    auto* b_settings=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_settings),"Original settings");
    lv_obj_add_event_cb(b_settings,[](lv_event_t*){ settings_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_wifi=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_wifi),"Original wifi");
    lv_obj_add_event_cb(b_wifi,[](lv_event_t*){ wifi_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_bluetooth=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_bluetooth),"Original bluetooth");
    lv_obj_add_event_cb(b_bluetooth,[](lv_event_t*){ bluetooth_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_meta=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_meta),"Original meta");
    lv_obj_add_event_cb(b_meta,[](lv_event_t*){ meta_screen_show(); },LV_EVENT_CLICKED,nullptr);
    auto* b_printer=lv_button_create(screen);
    lv_label_set_text(lv_label_create(b_printer),"Original printer");
    lv_obj_add_event_cb(b_printer,[](lv_event_t*){ printer_screen_show(); },LV_EVENT_CLICKED,nullptr);
}
}
int main() {
    SDL_SetMainReady();
    if(SDL_Init(SDL_INIT_VIDEO)!=0) { std::fprintf(stderr,"%s\n",SDL_GetError()); return 1; }
    auto* window=SDL_CreateWindow("T-Watch Ultra simulator",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H,0);
    if(window) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(renderer) texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,W,H);
    if(!texture) { std::fprintf(stderr,"%s\n",SDL_GetError()); if(renderer) SDL_DestroyRenderer(renderer); if(window) SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    lv_init();
    auto* display=lv_display_create(W,H);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    auto* input=lv_indev_create();
    lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,read_pointer);
    preview();
    stopwatch_screen_create();
    calendar_screen_create();
    timer_screen_create();
    alarm_screen_create();
    time_screen_create();
    usb_sd_screen_create();
    mouse_screen_create();
    webpanel_screen_create();
    pager_screen_create();
    tpms_screen_create();
    aprs_screen_create();
    adsb_screen_create();
    mic_screen_create();
    gps_screen_create();
    nodes_screen_create();
    configuration_screen_create();
    send_message_screen_create();
    channels_screen_create();
    meshtastic_screen_create();
    espnow_screen_create();
    lora_screen_create();
    wifi_radio_screen_create();
    settings_screen_create();
    wifi_screen_create();
    bluetooth_screen_create();
    meta_screen_create();
    printer_screen_create();
    bool running=true;
    uint32_t last=SDL_GetTicks();
    while(running) {
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_MOUSEMOTION) { pointer.x=e.motion.x; pointer.y=e.motion.y; }
            if((e.type==SDL_MOUSEBUTTONDOWN || e.type==SDL_MOUSEBUTTONUP) && e.button.button==SDL_BUTTON_LEFT) {
                pointer.x=e.button.x; pointer.y=e.button.y; pressed=e.type==SDL_MOUSEBUTTONDOWN;
            }
            if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_FOCUS_LOST) pressed=false;
            if(e.type==SDL_KEYDOWN && !e.key.repeat) {
                if(e.key.keysym.sym==SDLK_ESCAPE) running=false;
                if(e.key.keysym.sym==SDLK_p) { watch.button(sim::Button::power,true); watch.button(sim::Button::power,false); watch.set_display_on(!watch.display_on()); }
                if(e.key.keysym.sym==SDLK_b) { watch.button(sim::Button::back,true); watch.button(sim::Button::back,false); if(time_screen_is_active() || usb_sd_screen_is_active() || mouse_screen_is_active() || webpanel_screen_is_active() || pager_screen_is_active() || tpms_screen_is_active() || aprs_screen_is_active() || adsb_screen_is_active() || mic_screen_is_active() || gps_screen_is_active() || car_screen_is_active() || nodes_screen_is_active() || configuration_screen_is_active() || send_message_screen_is_active() || channels_screen_is_active() || meshtastic_screen_is_active() || espnow_screen_is_active() || lora_screen_is_active() || wifi_radio_screen_is_active() || calmati_remote_screen_is_active() || calmati_targets_screen_is_active() || calmati_result_screen_is_active() || settings_screen_is_active() || wifi_screen_is_active() || bluetooth_screen_is_active() || meta_screen_is_active() || printer_screen_is_active()) clock_screen_show(); else time_screen_show(); }
                if(e.key.keysym.sym==SDLK_m) { watch.set_motion(!watch.motion()); refresh(); }
            }
        }
        auto now=SDL_GetTicks(); auto delta=now-last; last=now;
        watch.advance(delta); lv_tick_inc(delta); lv_timer_handler();
        sim::ButtonEvent event{};
        while(watch.pop_button(event)) std::printf("button %d %s\n",static_cast<int>(event.button),event.pressed?"down":"up");
        SDL_UpdateTexture(texture,nullptr,pixels.data(),W*sizeof(uint16_t));
        SDL_SetRenderDrawColor(renderer,0,0,0,255); SDL_RenderClear(renderer);
        if(watch.display_on()) {
            auto level=static_cast<uint8_t>(watch.brightness());
            SDL_SetTextureColorMod(texture,level,level,level);
            SDL_RenderCopy(renderer,texture,nullptr,nullptr);
        }
        SDL_RenderPresent(renderer); SDL_Delay(10);
    }
    lv_indev_delete(input); lv_display_delete(display); lv_deinit();
    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    return 0;
}
