#pragma once
#include <SDL.h>
#include <lvgl.h>
#include <array>
#include "mock/hardware.hpp"
namespace sim {
// Independent SDL/LVGL surface; never changes the watch's default display.
class HardwarePanel {
public:
    static constexpr int width=420,height=780;
    SDL_Window* window=nullptr;
    lv_obj_t *battery=nullptr,*brightness=nullptr,*charging=nullptr,*display_on=nullptr;
    lv_obj_t *gps=nullptr,*satellites=nullptr,*sd=nullptr,*summary=nullptr,*core0=nullptr,*core1=nullptr;
    bool open(lv_display_t* watch_display);
    void close();
    bool event(const SDL_Event& e);
    void render();
    void show(){SDL_ShowWindow(window);SDL_RaiseWindow(window);}
    Uint32 id()const{return SDL_GetWindowID(window);}
private:
    SDL_Renderer* renderer=nullptr;
    SDL_Texture* texture=nullptr;
    lv_display_t* display=nullptr;
    lv_indev_t* input=nullptr;
    lv_point_t point{};bool pressed=false,touching=false;SDL_FingerID finger=0;
    std::array<uint16_t,width*height> pixels{};
    std::array<uint16_t,width*24> buffer{};
    static void changed(lv_event_t* e);
    void sync();
};
}
