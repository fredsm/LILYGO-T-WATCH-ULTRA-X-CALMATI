#pragma once
#include <lvgl.h>
#include "../tools_screen.h"
namespace sim {
inline void prepare_tools_scroll(){
    if(!tools_screen_is_active())return;
    auto* screen=lv_screen_active();
    lv_obj_update_layout(screen);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    for(unsigned i=0;i<lv_obj_get_child_count(screen);++i){
        auto* child=lv_obj_get_child(screen,i);
        if(!lv_obj_get_child_count(child))continue;
        // LVGL reserves a scroll object even when elastic scrolling is off.
        // Disable only outward directions at an edge so the original gesture
        // callback receives that motion. Inward movement still scrolls tiles.
        unsigned directions=0;
        if(lv_obj_get_scroll_top(child)>4)directions|=LV_DIR_TOP;
        if(lv_obj_get_scroll_bottom(child)>4)directions|=LV_DIR_BOTTOM;
        lv_obj_set_scroll_dir(child,static_cast<lv_dir_t>(directions));
    }
}
// A screen can change inside the original gesture callback. Keep the rest of
// that contact from being interpreted as a second swipe on the new screen.
inline void dispatch_pointer(lv_indev_t* input, bool pressed) {
    prepare_tools_scroll();
    auto* before=lv_screen_active();
    lv_indev_read(input);
    if(before!=lv_screen_active()){
        if(pressed)lv_indev_wait_release(input);
    }
}
}
