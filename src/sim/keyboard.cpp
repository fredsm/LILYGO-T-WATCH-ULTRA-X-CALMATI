#include "keyboard.hpp"
namespace {
struct Editor {lv_obj_t* area=nullptr;lv_obj_t* keyboard=nullptr;};
void find_editor(lv_obj_t* obj,Editor& result){
    if(lv_obj_has_flag(obj,LV_OBJ_FLAG_HIDDEN) || lv_obj_has_state(obj,LV_STATE_DISABLED))return;
    if(lv_obj_check_type(obj,&lv_keyboard_class)){
        auto* area=lv_keyboard_get_textarea(obj);
        if(area && lv_obj_is_visible(area) && !lv_obj_has_state(area,LV_STATE_DISABLED)){
            result.area=area;result.keyboard=obj;return;
        }
    }
    if(lv_obj_check_type(obj,&lv_textarea_class) && lv_obj_has_state(obj,LV_STATE_FOCUSED) && !result.keyboard)result.area=obj;
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)find_editor(lv_obj_get_child(obj,i),result);
}
Editor editor(){Editor result;find_editor(lv_screen_active(),result);find_editor(lv_layer_top(),result);return result;}
}
namespace sim {
bool text_input_active(){return editor().area!=nullptr;}
bool text_input(const char* utf8){auto e=editor();if(!e.area)return false;lv_textarea_add_text(e.area,utf8);return true;}
bool edit_key(uint32_t key){
    auto e=editor();if(!e.area)return false;
    if(e.keyboard && (key==LV_KEY_ENTER || key==LV_KEY_ESC)){
        lv_obj_send_event(e.keyboard,key==LV_KEY_ENTER?LV_EVENT_READY:LV_EVENT_CANCEL,nullptr);
    }else if(key==LV_KEY_ESC){
        lv_obj_remove_state(e.area,LV_STATE_FOCUSED);
        lv_obj_send_event(e.area,LV_EVENT_DEFOCUSED,nullptr);
    }else{
        lv_obj_send_event(e.area,LV_EVENT_KEY,&key);
    }
    return true;
}
}
