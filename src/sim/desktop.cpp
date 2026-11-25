#define SDL_MAIN_HANDLED
#include <SDL.h>
#include <lvgl.h>
#include "mock/watch.hpp"
#include "screens.hpp"
#include "clock.hpp"
#include "pointer.hpp"
#include "keyboard.hpp"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>
namespace {
constexpr int W=410,H=502;
constexpr int BAR=60;
std::array<uint16_t,W*(H+BAR)> pixels{};
std::array<uint16_t,W*32> buffer{};
std::array<uint16_t,W*20> controls_buffer{};
lv_display_t* controls_display=nullptr;
lv_obj_t* watch_buttons[2]{};
SDL_Texture* texture=nullptr;
SDL_Renderer* renderer=nullptr;
sim::Watch watch;
lv_point_t pointer{};
bool pressed=false;
lv_obj_t* status=nullptr;
lv_obj_t* screen_catalog=nullptr;
void flush(lv_display_t* d,const lv_area_t* a,uint8_t* data) {
    auto* src=reinterpret_cast<const uint16_t*>(data);
    const int width=a->x2-a->x1+1;
    for(int y=a->y1;y<=a->y2;++y)
        for(int x=a->x1;x<=a->x2;++x)
            if(x>=0 && x<W && y>=0 && y<H)
                pixels[(y+(d==controls_display?H:0))*W+x]=src[(y-a->y1)*width+x-a->x1];
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
    lv_obj_set_flex_flow(screen,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(screen,LV_FLEX_ALIGN_START,LV_FLEX_ALIGN_CENTER,LV_FLEX_ALIGN_CENTER);
    auto* title=lv_label_create(screen);
    lv_label_set_text(title,"T-Watch Ultra | SIMULATED DATA");
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
    std::size_t count=0;
    const auto* entries=sim::screens(count);
    for(std::size_t i=0;i<count;++i){
        auto* btn=lv_button_create(screen);
        lv_label_set_text(lv_label_create(btn),entries[i].name);
        lv_obj_add_event_cb(btn,[](lv_event_t* e){
            sim::leave_screen();
            static_cast<const sim::Screen*>(lv_event_get_user_data(e))->show();
        },LV_EVENT_CLICKED,const_cast<sim::Screen*>(&entries[i]));
    }
}
void create_button_bar(lv_display_t* main_display){
    controls_display=lv_display_create(W,BAR);
    lv_display_set_color_format(controls_display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(controls_display,controls_buffer.data(),nullptr,sizeof(controls_buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(controls_display,flush);
    auto* root=lv_display_get_screen_active(controls_display);
    lv_obj_set_style_pad_all(root,0,0);lv_obj_clear_flag(root,LV_OBJ_FLAG_SCROLLABLE);
    for(int i=0;i<2;++i){
        auto* button=lv_button_create(root);watch_buttons[i]=button;
        lv_obj_set_pos(button,10+i*205,8);lv_obj_set_size(button,185,44);
        auto* label=lv_label_create(button);
        lv_label_set_text(label,i==0?"Power (F2)":"Back (F3)");lv_obj_center(label);
    }
    lv_display_set_default(main_display);
}
uint32_t editing_key(SDL_Keycode key){
    switch(key){
    case SDLK_BACKSPACE:return LV_KEY_BACKSPACE;case SDLK_DELETE:return LV_KEY_DEL;
    case SDLK_LEFT:return LV_KEY_LEFT;case SDLK_RIGHT:return LV_KEY_RIGHT;
    case SDLK_UP:return LV_KEY_UP;case SDLK_DOWN:return LV_KEY_DOWN;
    case SDLK_HOME:return LV_KEY_HOME;case SDLK_END:return LV_KEY_END;
    case SDLK_RETURN:case SDLK_KP_ENTER:return LV_KEY_ENTER;case SDLK_ESCAPE:return LV_KEY_ESC;
    default:return 0;
    }
}
}
int main(int argc,char** argv) {
    const bool smoke=argc>1 && std::strcmp(argv[1],"--smoke")==0;
    SDL_SetMainReady();
    // Native touch is handled below; disable duplicate synthesized mouse events.
    SDL_SetHint(SDL_HINT_TOUCH_MOUSE_EVENTS,"0");
    SDL_SetHint(SDL_HINT_MOUSE_TOUCH_EVENTS,"0");
    if(SDL_Init(SDL_INIT_VIDEO)!=0) { std::fprintf(stderr,"%s\n",SDL_GetError()); return 1; }
    auto* window=SDL_CreateWindow("T-Watch Ultra | SIMULATED DATA | F1: catalog | Ctrl+Q: quit",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,W,H+BAR,0);
    if(window) renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(renderer) texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,W,H+BAR);
    if(!texture) { std::fprintf(stderr,"%s\n",SDL_GetError()); if(renderer) SDL_DestroyRenderer(renderer); if(window) SDL_DestroyWindow(window); SDL_Quit(); return 1; }
    lv_init();
    auto* display=lv_display_create(W,H);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,flush);
    auto* input=lv_indev_create();
    lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,read_pointer);
    // SDL supplies event samples. Timer polling between those samples would
    // inject zero-motion reads and repeatedly reset LVGL's gesture distance.
    lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);
    lv_indev_set_scroll_limit(input,35); // Same tap/drag threshold as firmware setup.
    preview();
    screen_catalog=lv_screen_active();
    sim::initialize_screens(sim::create_clock());
    create_button_bar(display);
    SDL_StartTextInput();
    std::size_t screen_count=0;const auto* screens=sim::screens(screen_count);
    unsigned frame=0;bool smoke_ok=true;
    bool running=true;
    uint32_t last=SDL_GetTicks();
    SDL_FingerID finger=0;bool touching=false;
    uint32_t pointer_event_at=last,held_read_at=last;
    auto dispatch_pointer=[&](){pointer_event_at=SDL_GetTicks();sim::dispatch_pointer(input,pressed);};
    bool contact=false;int captured_button=-1;
    auto route_pointer=[&](int x,int y,bool down){
        int hit=(y>=H+8 && y<H+52 && x>=10 && x<400)?(x<195?0:(x>=215?1:-1)):-1;
        if(down && !contact){contact=true;captured_button=hit;}
        if(captured_button>=0){
            auto* button=watch_buttons[captured_button];
            if(down && hit==captured_button)lv_obj_add_state(button,LV_STATE_PRESSED);
            else lv_obj_remove_state(button,LV_STATE_PRESSED);
            if(!down){
                if(hit==captured_button){if(captured_button==0)sim::power_button_click();else sim::back_button_click();}
                captured_button=-1;contact=false;
            }
            return;
        }
        if(!contact && y>=H)return;
        pointer.x=std::clamp(x,0,W-1);pointer.y=std::clamp(y,0,H-1);
        pressed=down;dispatch_pointer();if(!down)contact=false;
    };
    while(running) {
        if(smoke && frame%5==0){
            const auto index=frame/5;
            sim::show_home();
            if(index>=screen_count)break;
            screens[index].show();
            if(!screens[index].active()){smoke_ok=false;break;}
        }
        ++frame;
        SDL_Event e;
        while(SDL_PollEvent(&e)) {
            if(e.type==SDL_QUIT) running=false;
            if(e.type==SDL_MOUSEMOTION && e.motion.which!=SDL_TOUCH_MOUSEID && !touching) {
                route_pointer(e.motion.x,e.motion.y,contact);
            }
            if((e.type==SDL_MOUSEBUTTONDOWN || e.type==SDL_MOUSEBUTTONUP) && e.button.button==SDL_BUTTON_LEFT && e.button.which!=SDL_TOUCH_MOUSEID && !touching) {
                route_pointer(e.button.x,e.button.y,e.type==SDL_MOUSEBUTTONDOWN);
            }
            if(e.type==SDL_FINGERDOWN && !touching){finger=e.tfinger.fingerId;touching=true;}
            if(touching && (e.type==SDL_FINGERDOWN || e.type==SDL_FINGERMOTION || e.type==SDL_FINGERUP) && e.tfinger.fingerId==finger){
                route_pointer(static_cast<int>(e.tfinger.x*(W-1)),static_cast<int>(e.tfinger.y*(H+BAR-1)),e.type!=SDL_FINGERUP);
                if(e.type==SDL_FINGERUP)touching=false;
            }
            if(e.type==SDL_WINDOWEVENT && e.window.event==SDL_WINDOWEVENT_FOCUS_LOST) {
                pressed=false;touching=false;contact=false;captured_button=-1;
                for(auto* b:watch_buttons)lv_obj_remove_state(b,LV_STATE_PRESSED);dispatch_pointer();
            }
            if(e.type==SDL_TEXTINPUT)sim::text_input(e.text.text);
            if(e.type==SDL_KEYDOWN) {
                const auto key=e.key.keysym.sym;const bool ctrl=(e.key.keysym.mod&KMOD_CTRL)!=0;
                if(ctrl && key==SDLK_q){running=false;continue;}
                if(sim::text_input_active()){
                    if(ctrl && key==SDLK_v){char* text=SDL_GetClipboardText();if(text){sim::text_input(text);SDL_free(text);}continue;}
                    if(auto edit=editing_key(key)){sim::edit_key(edit);continue;}
                }
                if(e.key.repeat)continue;
                const bool editing=sim::text_input_active();
                if(key==SDLK_F2 || (!editing && key==SDLK_p))sim::power_button_click();
                if(key==SDLK_F3 || (!editing && (key==SDLK_b || key==SDLK_ESCAPE)))sim::back_button_click();
                if(key==SDLK_F4)sim::show_home();
                if(key==SDLK_F5)watch.set_display_on(!watch.display_on());
                if(key==SDLK_F1) { sim::leave_screen(); lv_screen_load(screen_catalog); }
                if(!editing && key==SDLK_m) { watch.set_motion(!watch.motion()); refresh(); }
            }
        }
        auto now=SDL_GetTicks(); auto delta=smoke?20u:now-last; last=now;
        watch.advance(delta); lv_tick_inc(delta); sim::tick();
        // Stationary holds still need reads for original long-press callbacks.
        if(pressed && now-pointer_event_at>=100 && now-held_read_at>=30){
            sim::dispatch_pointer(input,pressed);held_read_at=now;
        }
        lv_timer_handler();
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
    SDL_StopTextInput();lv_indev_delete(input);lv_display_delete(controls_display);lv_display_delete(display);lv_deinit();
    SDL_DestroyTexture(texture); SDL_DestroyRenderer(renderer); SDL_DestroyWindow(window); SDL_Quit();
    return smoke_ok?0:2;
}
