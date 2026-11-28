#include "keyboard_manager.h"
#include "watch_display_geometry.h"
#include <new>

namespace {
struct Session {
    lv_obj_t* keyboard=nullptr;
    lv_obj_t* screen=nullptr;
    lv_obj_t* field=nullptr;
    lv_align_t align=LV_ALIGN_DEFAULT;
    int32_t x=0,y=0,w=0,h=0,index=0;
    bool moved=false;
    Session* next=nullptr;
};
Session* sessions=nullptr;
Session* find(lv_obj_t* kb){
    for(auto* s=sessions;s;s=s->next)if(s->keyboard==kb)return s;
    return nullptr;
}
int32_t clamp(int32_t value,int32_t lo,int32_t hi){
    return value<lo?lo:value>hi?hi:value;
}
void on_field_delete(lv_event_t* e){
    auto* s=static_cast<Session*>(lv_event_get_user_data(e));
    // The textarea is still live during DELETE; detach before it is freed.
    s->field=nullptr;s->moved=false;
    lv_obj_add_flag(s->keyboard,LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(s->keyboard,nullptr);
}
void restore(Session* s){
    if(!s||!s->field)return;
    auto* field=s->field;
    s->field=nullptr;
    if(!lv_obj_is_valid(field)){s->moved=false;return;}
    lv_obj_remove_event_cb_with_user_data(field,on_field_delete,s);
    if(s->moved){
        lv_obj_set_size(field,s->w,s->h);
        lv_obj_align(field,s->align,s->x,s->y);
        lv_obj_move_to_index(field,s->index);
    }
    s->moved=false;
}
void on_unload(lv_event_t* e){
    auto* s=static_cast<Session*>(lv_event_get_user_data(e));
    watch_keyboard_hide(s->keyboard);
}
void on_delete(lv_event_t* e){
    auto* s=static_cast<Session*>(lv_event_get_user_data(e));
    if(lv_obj_is_valid(s->screen))lv_obj_remove_event_cb_with_user_data(s->screen,on_unload,s);
    if(s->field&&lv_obj_is_valid(s->field))lv_obj_clear_state(s->field,LV_STATE_FOCUSED);
    restore(s);
    Session** link=&sessions;
    while(*link&&*link!=s)link=&(*link)->next;
    if(*link)*link=s->next;
    delete s;
}
void keep_field_visible(Session* s,lv_obj_t* field){
    if(!s||!field)return;
    lv_obj_update_layout(s->screen);
    lv_area_t area,kb,parent;
    lv_obj_get_coords(field,&area);
    lv_obj_get_coords(s->keyboard,&kb);
    lv_obj_get_content_coords(lv_obj_get_parent(field),&parent);
    const int32_t left=parent.x1>WATCH_KEYBOARD_SIDE_INSET?parent.x1:WATCH_KEYBOARD_SIDE_INSET;
    const int32_t right=parent.x2<WATCH_DISPLAY_WIDTH-WATCH_KEYBOARD_SIDE_INSET-1?
        parent.x2:WATCH_DISPLAY_WIDTH-WATCH_KEYBOARD_SIDE_INSET-1;
    const int32_t top=parent.y1>WATCH_KEYBOARD_TOP_INSET?parent.y1:WATCH_KEYBOARD_TOP_INSET;
    const int32_t bottom=parent.y2<kb.y1-10?parent.y2:kb.y1-10;
    if(right<left||bottom<top)return; // field's own container has no usable area
    int32_t w=lv_area_get_width(&area),h=lv_area_get_height(&area);
    if(w>right-left+1)w=right-left+1;
    if(h>bottom-top+1)h=bottom-top+1;
    const int32_t x=clamp(area.x1,left,right-w+1);
    const int32_t y=clamp(area.y1,top,bottom-h+1);
    if(x==area.x1&&y==area.y1&&w==lv_area_get_width(&area)&&h==lv_area_get_height(&area))return;
    s->align=lv_obj_get_style_align(field,LV_PART_MAIN);
    s->x=lv_obj_get_style_x(field,LV_PART_MAIN);
    s->y=lv_obj_get_style_y(field,LV_PART_MAIN);
    s->w=lv_obj_get_style_width(field,LV_PART_MAIN);
    s->h=lv_obj_get_style_height(field,LV_PART_MAIN);
    s->index=lv_obj_get_index(field);
    s->moved=true;
    lv_obj_set_size(field,w,h);
    lv_obj_align_to(field,s->screen,LV_ALIGN_TOP_LEFT,x,y);
    lv_obj_move_foreground(field);
}
}
lv_obj_t* watch_keyboard_create(lv_obj_t* parent,KeyboardProfile profile){
    auto* kb=lv_keyboard_create(parent);
    const int height=profile==KeyboardProfile::Compose?300:profile==KeyboardProfile::Compact?200:220;
    lv_obj_set_size(kb,WATCH_DISPLAY_WIDTH-2*WATCH_KEYBOARD_SIDE_INSET,height);
    lv_obj_align(kb,LV_ALIGN_BOTTOM_MID,0,-WATCH_KEYBOARD_BOTTOM_INSET);
    lv_obj_add_flag(kb,LV_OBJ_FLAG_FLOATING);
    lv_obj_clear_flag(kb,LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_pad_all(kb,4,LV_PART_MAIN);
    lv_obj_set_style_pad_row(kb,4,LV_PART_MAIN);
    lv_obj_set_style_pad_column(kb,2,LV_PART_MAIN);
    lv_obj_set_style_radius(kb,10,LV_PART_MAIN);
    lv_obj_set_style_border_width(kb,0,LV_PART_MAIN);
    lv_obj_set_style_text_font(kb,&lv_font_montserrat_16,LV_PART_ITEMS);
    lv_obj_set_style_pad_all(kb,1,LV_PART_ITEMS);
    lv_obj_set_style_radius(kb,5,LV_PART_ITEMS);
    lv_keyboard_set_popovers(kb,false);
    lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
    auto* s=new(std::nothrow) Session;
    if(s){
        s->keyboard=kb;s->screen=lv_obj_get_screen(kb);
        s->next=sessions;sessions=s;
        lv_obj_add_event_cb(kb,on_delete,LV_EVENT_DELETE,s);
        lv_obj_add_event_cb(s->screen,on_unload,LV_EVENT_SCREEN_UNLOADED,s);
    }
    return kb;
}
void watch_keyboard_show(lv_obj_t* kb,lv_obj_t* field){
    if(!kb||!field)return;
    auto* s=find(kb);
    if(s&&s->field!=field){
        restore(s);s->field=field;
        lv_obj_add_event_cb(field,on_field_delete,LV_EVENT_DELETE,s);
        lv_keyboard_set_textarea(kb,field);
        lv_obj_clear_flag(kb,LV_OBJ_FLAG_HIDDEN);
        keep_field_visible(s,field);
    }else{
        lv_keyboard_set_textarea(kb,field);
        lv_obj_clear_flag(kb,LV_OBJ_FLAG_HIDDEN);
    }
    lv_obj_move_foreground(kb);
}
void watch_keyboard_hide(lv_obj_t* kb){
    if(!kb)return;
    lv_obj_add_flag(kb,LV_OBJ_FLAG_HIDDEN);
    lv_keyboard_set_textarea(kb,nullptr);
    restore(find(kb));
}
