#pragma once
#include <lvgl.h>
#include "../tools_screen.h"
namespace sim {
// A screen can change inside the original gesture callback. Keep the rest of
// that contact from being interpreted as a second swipe on the new screen.
inline void dispatch_pointer(lv_indev_t* input, bool pressed) {
    auto* before=lv_screen_active();
    lv_indev_read(input);
    if(before!=lv_screen_active()){
        if(tools_screen_is_active()){
            // Let the original Tools edge-gesture handler see an outward swipe.
            // Elastic overscroll otherwise consumes it even at the grid boundary.
            // Keep normal scrolling of the original tile grid intact.
            auto* screen=lv_screen_active();
            lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
            for(unsigned i=0;i<lv_obj_get_child_count(screen);++i){
                auto* child=lv_obj_get_child(screen,i);
                if(lv_obj_get_child_count(child) && lv_obj_get_scroll_dir(child)==LV_DIR_VER)
                    lv_obj_clear_flag(child,LV_OBJ_FLAG_SCROLL_ELASTIC|LV_OBJ_FLAG_SCROLL_CHAIN_VER);
            }
        }
        if(pressed)lv_indev_wait_release(input);
    }
}
}
