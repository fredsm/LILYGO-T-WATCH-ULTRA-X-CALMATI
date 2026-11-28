#pragma once

// Portrait logical coordinates of the Ultra AMOLED. The mask radius is a
// conservative simulator approximation, not a calibrated panel measurement.
constexpr int WATCH_DISPLAY_WIDTH=410;
constexpr int WATCH_DISPLAY_HEIGHT=502;
constexpr int WATCH_DISPLAY_CORNER_RADIUS=90;
constexpr int WATCH_KEYBOARD_SIDE_INSET=30;
constexpr int WATCH_KEYBOARD_BOTTOM_INSET=44;
constexpr int WATCH_KEYBOARD_TOP_INSET=40;

inline bool watch_display_contains(int x,int y){
    if(x<0||y<0||x>=WATCH_DISPLAY_WIDTH||y>=WATCH_DISPLAY_HEIGHT)return false;
    constexpr int r=WATCH_DISPLAY_CORNER_RADIUS;
    int dx=x<r?r-x:x>=WATCH_DISPLAY_WIDTH-r?x-(WATCH_DISPLAY_WIDTH-r-1):0;
    int dy=y<r?r-y:y>=WATCH_DISPLAY_HEIGHT-r?y-(WATCH_DISPLAY_HEIGHT-r-1):0;
    return dx==0||dy==0||dx*dx+dy*dy<=r*r;
}
