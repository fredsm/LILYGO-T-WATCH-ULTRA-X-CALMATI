#include "../../src/layout_config.h"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/sim/mock/SD.h"
#include "../../src/quick_controls_screen.h"
#include "../../src/tools_screen.h"
#include "../../src/settings_screen.h"
#include "../../src/time_screen.h"
#include "../../src/gps_screen.h"
#include "../../src/calendar_screen.h"
#include "../../src/pin_lock_screen.h"
#include "../../src/power_manager.h"
#include <array>
#include <cstdio>
#include <cstring>
#include <string>
#define CHECK(x) do{if(!(x)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#x);return 1;}}while(0)
namespace {
bool parse(const char* value,LayoutConfig& dest){
    std::string s(value);s.push_back(0);return layout_parse(s.data(),s.size()-1,dest);
}
lv_indev_t* input=nullptr;lv_point_t point{};bool pressed=false;
void pump(){for(int i=0;i<15;++i){lv_tick_inc(20);sim::tick();lv_timer_handler();}}
void sample(int x,int y,bool down){point={x,y};pressed=down;lv_tick_inc(20);sim::dispatch_pointer(input,pressed);lv_timer_handler();}
void swipe(int x,int y,int dx,int dy){
    sample(x,y,true);for(int i=1;i<=8;++i)sample(x+dx*i/8,y+dy*i/8,true);
    sample(x+dx,y+dy,false);pump();
}
lv_obj_t* label(lv_obj_t* root,const char* value){
    if(lv_obj_check_type(root,&lv_label_class)&&!strcmp(lv_label_get_text(root),value))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);++i)
        if(auto* o=label(lv_obj_get_child(root,i),value))return o;
    return nullptr;
}
void save(const char* text,const char* path=LAYOUT_PATH){auto f=SD.open(path,FILE_WRITE);f.print(text);f.close();}
constexpr const char* custom=
    "# user config\r\n"
    "home.swipe_left=tools\r\n"
    "home.swipe_right=time\n"
    "home.swipe_up=quick_controls\n"
    "home.swipe_down=gps\n"
    "home.back=calendar\n"
    "home.power=settings\n"
    "tools.order=Task Mgr,Time\n"
    "tools.hide=Mouse\n"
    "quick.controls=brightness,gps,wifi\n"
    "quick.actions=lock,settings\n";
}
int main(){
    CHECK(layout_embedded_valid());
    auto c=layout_defaults();
    CHECK(c.home[0]==HomeTarget::Time&&c.home[5]==HomeTarget::Gps);
    CHECK(c.tool_count==LAYOUT_TOOL_COUNT&&c.control_count==6&&c.action_count==2);
    CHECK(parse(custom,c));
    CHECK(c.home[0]==HomeTarget::Tools&&c.home[5]==HomeTarget::Settings);
    CHECK(!strcmp(layout_tool_name(c.tools[0]),"Task Mgr"));
    CHECK(!strcmp(layout_tool_name(c.tools[1]),"Time"));
    CHECK(!strcmp(layout_tool_name(c.tools[2]),"Wardriver")); // unlisted tools follow ordered entries
    CHECK(!strcmp(layout_tool_name(c.tools[c.tool_count-1]),"WPA3 SAE"));
    CHECK(c.controls[0]==5&&c.control_count==3&&c.actions[0]==1);
    // Reject whole files without changing the caller's last good configuration.
    for(auto* bad:{"home.power=not_a_screen","home.power=time\nhome.power=gps",
        "tools.order=Time,Time","tools.hide=unknown","quick.controls=gps,","quick.actions=lock,lock",
        "bad=key","home.back=none\nhome.swipe_down=none","home.back=none\nquick.actions=lock"}){
        CHECK(!parse(bad,c));CHECK(c.home[5]==HomeTarget::Settings);
    }
    CHECK(parse("home.power=none\nquick.controls=\n",c));
    CHECK(c.home[5]==HomeTarget::None&&c.control_count==0&&c.home[0]==HomeTarget::Time);
    std::string huge(LAYOUT_MAX_BYTES+2,'#');CHECK(!layout_parse(huge.data(),huge.size()-1,c));
    char nul[]={'a',0,'b',0};CHECK(!layout_parse(nul,3,c));

    lv_init();auto* d=lv_display_create(410,502);
    static std::array<uint16_t,410*32> buf{};
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buf.data(),nullptr,sizeof(buf),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,[](lv_display_t* disp,const lv_area_t*,uint8_t*){lv_display_flush_ready(disp);});
    input=lv_indev_create();lv_indev_set_type(input,LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(input,[](lv_indev_t*,lv_indev_data_t* data){data->point=point;data->state=pressed?LV_INDEV_STATE_PRESSED:LV_INDEV_STATE_RELEASED;});
    lv_indev_set_mode(input,LV_INDEV_MODE_EVENT);lv_indev_set_scroll_limit(input,35);
    save("home.power=timer\n",LAYOUT_DEFAULT_PATH);
    save(custom);
    auto* home=sim::create_clock();sim::initialize_screens(home);power_init();pump();
    CHECK(layout_status().source==LayoutSource::CustomFile);
    swipe(330,250,-240,0);CHECK(tools_screen_is_active());
    auto* first=label(lv_screen_active(),"Task Mgr");CHECK(first&&lv_obj_get_index(lv_obj_get_parent(first))==0);
    auto* time=label(lv_screen_active(),"Time");CHECK(time&&lv_obj_get_index(lv_obj_get_parent(time))==1);
    auto* mouse=label(lv_screen_active(),"Mouse");CHECK(mouse&&lv_obj_has_flag(lv_obj_get_parent(mouse),LV_OBJ_FLAG_HIDDEN));
    auto* grid=lv_obj_get_parent(lv_obj_get_parent(first));
    CHECK(lv_obj_get_child_count(grid)==LAYOUT_TOOL_COUNT);
    sim::show_home();pump();swipe(80,250,240,0);CHECK(time_screen_is_active());
    sim::show_home();pump();swipe(205,330,0,-240);CHECK(quick_controls_screen_is_active());
    auto* brightness=label(lv_screen_active(),"Brightness");CHECK(brightness);
    auto* tile=lv_obj_get_parent(brightness);CHECK(lv_obj_get_x(tile)==35&&lv_obj_get_y(tile)==76);
    auto* bluetooth=label(lv_screen_active(),"Bluetooth");CHECK(bluetooth&&lv_obj_has_flag(lv_obj_get_parent(bluetooth),LV_OBJ_FLAG_HIDDEN));
    auto* lock=label(lv_screen_active(),"Lock");CHECK(lock&&lv_obj_get_x(lv_obj_get_parent(lock))==35);
    sim::show_home();pump();swipe(205,100,0,240);CHECK(gps_screen_is_active());
    sim::show_home();pump();sim::back_button_click();CHECK(calendar_screen_is_active());
    sim::show_home();pump();sim::power_button_click();CHECK(settings_screen_is_active());
    // Edits are boot-only; changing the SD file does not move live controls.
    save("home.power=bad");CHECK(layout_status().source==LayoutSource::CustomFile);
    sim::show_home();pump();sim::power_button_click();CHECK(settings_screen_is_active());
    sim::show_home();pin_lock_activate();layout_open_home(HomeInput::Power);
    CHECK(lv_screen_active()==home);
    // Deep reboot restores the SD default for a malformed custom file.
    deepsleep();CHECK(power_is_asleep());power_button_down(10000);
    power_tick(15000);power_button_up(15100);
    CHECK(layout_status().source==LayoutSource::DefaultFile&&layout_status().invalid_custom);
    CHECK(layout_config().home[5]==HomeTarget::Timer);
    CHECK(!lv_obj_has_flag(lv_obj_get_parent(mouse),LV_OBJ_FLAG_HIDDEN));
    CHECK(lv_obj_get_index(lv_obj_get_parent(time))==0);
    CHECK(!lv_obj_has_flag(lv_obj_get_parent(bluetooth),LV_OBJ_FLAG_HIDDEN));
    // Valid partial custom config inherits SD default fields and ordering.
    save("home.back=diagnostics\ntools.order=Task Mgr\ntools.hide=Mouse",LAYOUT_DEFAULT_PATH);
    save("home.power=settings\ntools.order=Calendar",LAYOUT_PATH);
    layout_load();CHECK(layout_status().source==LayoutSource::DefaultFile&&layout_status().invalid_custom);
    save("home.power=settings\ntools.order=Time\ntools.hide=",LAYOUT_PATH);
    layout_load();CHECK(layout_status().source==LayoutSource::CustomFile&&!layout_status().invalid_custom);
    CHECK(layout_config().home[4]==HomeTarget::Diagnostics&&layout_config().home[5]==HomeTarget::Settings);
    CHECK(!strcmp(layout_tool_name(layout_config().tools[0]),"Time"));
    CHECK(!strcmp(layout_tool_name(layout_config().tools[1]),"Task Mgr"));
    for(unsigned i=0;i<LAYOUT_TOOL_COUNT;++i)CHECK(!layout_config().hidden[i]);
    sim_sd_files.erase(LAYOUT_PATH);layout_load();
    CHECK(layout_status().source==LayoutSource::DefaultFile&&!layout_status().invalid_custom);
    CHECK(layout_config().home[4]==HomeTarget::Diagnostics);
    save("home.power=settings",LAYOUT_PATH);
    sim_sd_files.erase(LAYOUT_DEFAULT_PATH);layout_load();
    CHECK(layout_status().source==LayoutSource::CustomFile&&!layout_status().invalid_default);
    CHECK(layout_config().home[4]==HomeTarget::Settings);
    save("home.power=bad",LAYOUT_DEFAULT_PATH);
    layout_load();CHECK(layout_status().source==LayoutSource::CustomFile&&layout_status().invalid_default);
    CHECK(layout_config().home[4]==HomeTarget::Settings);
    save("home.power=bad",LAYOUT_PATH);
    layout_load();CHECK(layout_status().source==LayoutSource::Embedded&&layout_status().invalid_default&&layout_status().invalid_custom);
    CHECK(layout_config().home[5]==HomeTarget::Gps);
    sim::hardware.sd_ready=false;layout_load();CHECK(layout_status().source==LayoutSource::Embedded);
    CHECK(!layout_status().invalid_default&&!layout_status().invalid_custom);
    return 0;
}
