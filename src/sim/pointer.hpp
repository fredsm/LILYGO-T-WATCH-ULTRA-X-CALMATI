#pragma once
#include <lvgl.h>
namespace sim {
// A screen can change inside the original gesture callback. Keep the rest of
// that contact from being interpreted as a second swipe on the new screen.
inline void dispatch_pointer(lv_indev_t* input, bool pressed) {
    auto* before=lv_screen_active();
    lv_indev_read(input);
    if(pressed && before!=lv_screen_active())lv_indev_wait_release(input);
}
}
