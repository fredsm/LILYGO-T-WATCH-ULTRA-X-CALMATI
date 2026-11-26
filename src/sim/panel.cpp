#include "panel.hpp"
#include <algorithm>
namespace sim {
bool HardwarePanel::open(lv_display_t* watch_display){
    window=SDL_CreateWindow("Hardware simulation | F6 reopens this panel",SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,width,height,0);
    if(window)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
    if(renderer)texture=SDL_CreateTexture(renderer,SDL_PIXELFORMAT_RGB565,SDL_TEXTUREACCESS_STREAMING,width,height);
    if(!texture){close();return false;}
    display=lv_display_create(width,height);
    lv_display_set_user_data(display,this);
    lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display,[](lv_display_t* d,const lv_area_t* a,uint8_t* data){
        auto* self=static_cast<HardwarePanel*>(lv_display_get_user_data(d));
        auto* src=reinterpret_cast<uint16_t*>(data);int stride=a->x2-a->x1+1;
        for(int y=a->y1;y<=a->y2;++y)for(int x=a->x1;x<=a->x2;++x)
            if(x>=0&&x<width&&y>=0&&y<height)self->pixels[y*width+x]=src[(y-a->y1)*stride+x-a->x1];
        lv_display_flush_ready(d);
    });
    input=lv_indev_create();lv_indev_set_display(input,display);
    lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);
    lv_indev_set_user_data(input,this);
    lv_indev_set_read_cb(input,[](lv_indev_t* i,lv_indev_data_t* d){
        auto* self=static_cast<HardwarePanel*>(lv_indev_get_user_data(i));
        d->point=self->point;d->state=self->pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;
    });
    auto* root=lv_display_get_screen_active(display);
    lv_obj_set_style_pad_all(root,18,0);lv_obj_set_flex_flow(root,LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_row(root,12,0);
    lv_label_set_text(lv_label_create(root),"SIMULATED DATA - session only");
    summary=lv_label_create(root);
    auto slider=[&](const char* label,int maximum){
        lv_label_set_text(lv_label_create(root),label);auto* s=lv_slider_create(root);
        lv_obj_set_size(s,350,16);lv_slider_set_range(s,0,maximum);
        lv_obj_add_event_cb(s,changed,LV_EVENT_VALUE_CHANGED,this);return s;
    };
    battery=slider("Battery (%)",100);brightness=slider("Brightness (0-255)",255);
    satellites=slider("GPS satellites",32);
    auto checkbox=[&](const char* label){auto* c=lv_checkbox_create(root);lv_checkbox_set_text(c,label);
        lv_obj_add_event_cb(c,changed,LV_EVENT_VALUE_CHANGED,this);return c;};
    charging=checkbox("Charging");display_on=checkbox("Watch display on");
    gps=checkbox("GPS fix available");sd=checkbox("SD card present (virtual)");
    auto* reset=lv_button_create(root);lv_label_set_text(lv_label_create(reset),"Reset simulated inputs");
    lv_obj_add_event_cb(reset,[](lv_event_t* e){hardware.reset_inputs();static_cast<HardwarePanel*>(lv_event_get_user_data(e))->sync();},LV_EVENT_CLICKED,this);
    auto* note=lv_label_create(root);lv_obj_set_width(note,370);lv_label_set_long_mode(note,LV_LABEL_LONG_WRAP);
    lv_label_set_text(note,"GPS/SD changes appear when the original UI refreshes or reopens. No real devices, files or radio traffic. Close hides this panel; F6 reopens it.");
    sync();lv_display_set_default(watch_display);return true;
}
void HardwarePanel::changed(lv_event_t* e){
    auto* p=static_cast<HardwarePanel*>(lv_event_get_user_data(e));auto* target=static_cast<lv_obj_t*>(lv_event_get_target(e));
    if(target==p->battery)hardware.watch.set_battery(lv_slider_get_value(target));
    if(target==p->brightness)hardware.watch.set_brightness(lv_slider_get_value(target));
    if(target==p->satellites)hardware.satellites=lv_slider_get_value(target);
    bool checked=lv_obj_has_state(target,LV_STATE_CHECKED);
    if(target==p->charging)hardware.watch.set_charging(checked);
    if(target==p->display_on)hardware.watch.set_display_on(checked);
    if(target==p->gps)hardware.gps_fix=checked;
    if(target==p->sd)hardware.sd_ready=checked;
}
void HardwarePanel::sync(){
    lv_slider_set_value(battery,hardware.watch.battery(),LV_ANIM_OFF);
    lv_slider_set_value(brightness,hardware.watch.brightness(),LV_ANIM_OFF);
    lv_slider_set_value(satellites,hardware.satellites,LV_ANIM_OFF);
    auto check=[](lv_obj_t* obj,bool value){if(value)lv_obj_add_state(obj,LV_STATE_CHECKED);else lv_obj_remove_state(obj,LV_STATE_CHECKED);};
    check(charging,hardware.watch.charging());check(display_on,hardware.watch.display_on());
    check(gps,hardware.gps_fix);check(sd,hardware.sd_ready);
    lv_label_set_text_fmt(summary,"Battery %d%% | Brightness %d%%\nGPS %s | %d satellites",hardware.watch.battery(),hardware.watch.brightness()*100/255,hardware.gps_fix?"fix":"no fix",hardware.satellites);
}
bool HardwarePanel::event(const SDL_Event& e){
    Uint32 target=0;
    switch(e.type){
    case SDL_MOUSEMOTION:target=e.motion.windowID;break;
    case SDL_MOUSEBUTTONDOWN:case SDL_MOUSEBUTTONUP:target=e.button.windowID;break;
    case SDL_MOUSEWHEEL:target=e.wheel.windowID;break;
    case SDL_FINGERDOWN:case SDL_FINGERUP:case SDL_FINGERMOTION:target=e.tfinger.windowID;break;
    case SDL_KEYDOWN:case SDL_KEYUP:target=e.key.windowID;break;
    case SDL_TEXTINPUT:target=e.text.windowID;break;
    case SDL_WINDOWEVENT:target=e.window.windowID;break;
    default:return false;
    }
    if(target!=id())return false;
    auto sample=[&](int x,int y,bool down){point={std::clamp(x,0,width-1),std::clamp(y,0,height-1)};pressed=down;lv_indev_read(input);};
    if(e.type==SDL_WINDOWEVENT){
        if(e.window.event==SDL_WINDOWEVENT_CLOSE)SDL_HideWindow(window);
        if(e.window.event==SDL_WINDOWEVENT_CLOSE||e.window.event==SDL_WINDOWEVENT_FOCUS_LOST){touching=false;sample(point.x,point.y,false);}
    }
    if(e.type==SDL_MOUSEMOTION&&!touching&&e.motion.which!=SDL_TOUCH_MOUSEID)sample(e.motion.x,e.motion.y,pressed);
    if((e.type==SDL_MOUSEBUTTONDOWN||e.type==SDL_MOUSEBUTTONUP)&&!touching&&e.button.which!=SDL_TOUCH_MOUSEID&&e.button.button==SDL_BUTTON_LEFT)sample(e.button.x,e.button.y,e.type==SDL_MOUSEBUTTONDOWN);
    if(e.type==SDL_FINGERDOWN&&!touching){touching=true;finger=e.tfinger.fingerId;}
    if(touching&&(e.type==SDL_FINGERDOWN||e.type==SDL_FINGERMOTION||e.type==SDL_FINGERUP)&&finger==e.tfinger.fingerId){
        sample(int(e.tfinger.x*(width-1)),int(e.tfinger.y*(height-1)),e.type!=SDL_FINGERUP);if(e.type==SDL_FINGERUP)touching=false;
    }
    // Consume panel keyboard/text events so they cannot edit or navigate the watch.
    return true;
}
void HardwarePanel::render(){
    sync();SDL_UpdateTexture(texture,nullptr,pixels.data(),width*sizeof(uint16_t));
    SDL_RenderClear(renderer);SDL_RenderCopy(renderer,texture,nullptr,nullptr);SDL_RenderPresent(renderer);
}
void HardwarePanel::close(){
    if(input)lv_indev_delete(input);if(display)lv_display_delete(display);
    input=nullptr;display=nullptr;
    if(texture)SDL_DestroyTexture(texture);if(renderer)SDL_DestroyRenderer(renderer);if(window)SDL_DestroyWindow(window);
    texture=nullptr;renderer=nullptr;window=nullptr;
}
}
