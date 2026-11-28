#include "../../src/layout_config.h"
#include "../../src/layout_remote.h"
#include "../../src/radio_manager.h"
#include "../../src/sim/mock/WiFi.h"
#include "../../src/sim/screens.hpp"
#include "../../src/sim/clock.hpp"
#include "../../src/sim/pointer.hpp"
#include "../../src/sim/mock/SD.h"
#include "../../src/sim/mock/FFat.h"
#include "../../src/storage_manager.h"
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
lv_obj_t* textarea(lv_obj_t* root){
    if(lv_obj_check_type(root,&lv_textarea_class))return root;
    for(unsigned i=0;i<lv_obj_get_child_count(root);++i)if(auto* o=textarea(lv_obj_get_child(root,i)))return o;
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
    // Inventory keys are informational and do not grant access to an unknown target.
    CHECK(parse("available-screens=home,time\navailable-home-targets=none,time\nhome.power=none\nquick.controls=\n",c));
    CHECK(!parse("available-home-targets=unregistered\nhome.power=unregistered",c));
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
    // SD edits must not influence initial boot.
    save("home.power=timer\n",LAYOUT_DEFAULT_PATH);
    save(custom);
    auto* home=sim::create_clock();sim::initialize_screens(home);power_init();pump();
    CHECK(layout_status().source==LayoutSource::Embedded&&layout_status().flash_ready);
    CHECK(layout_config().home[5]==HomeTarget::Gps);
    // Import using the actual Settings button; apply only after validation/persistence.
    settings_screen_show();pump();
    auto* import=label(lv_screen_active(),"Import layout from SD");CHECK(import);
    lv_obj_send_event(lv_obj_get_parent(import),LV_EVENT_CLICKED,nullptr);
    CHECK(layout_status().source==LayoutSource::Internal);
    CHECK(FFat.exists(LAYOUT_PATH));
    sim::show_home();pump();
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
    // Editing SD after import cannot change the active layout or boot.
    save("home.power=bad");CHECK(layout_status().source==LayoutSource::Internal);
    CHECK(layout_import_sd()==LayoutResult::InvalidFile);
    sim::show_home();pump();sim::power_button_click();CHECK(settings_screen_is_active());
    sim::show_home();pin_lock_activate();layout_open_home(HomeInput::Power);
    CHECK(lv_screen_active()==home);
    deepsleep();CHECK(power_is_asleep());power_button_down(10000);
    power_tick(15000);power_button_up(15100);
    CHECK(layout_status().source==LayoutSource::Internal);
    CHECK(layout_config().home[5]==HomeTarget::Settings);
    CHECK(lv_obj_has_flag(lv_obj_get_parent(mouse),LV_OBJ_FLAG_HIDDEN));
    CHECK(lv_obj_get_index(lv_obj_get_parent(time))==1);
    CHECK(lv_obj_has_flag(lv_obj_get_parent(bluetooth),LV_OBJ_FLAG_HIDDEN));

    // A new validated file replaces internal storage, and retains previous good file.
    save("home.power=timer\ntools.order=Time");
    CHECK(layout_import_sd()==LayoutResult::Ok);
    CHECK(layout_config().home[5]==HomeTarget::Timer&&FFat.exists("/Settings/layout.bak"));
    CHECK(!lv_obj_has_flag(lv_obj_get_parent(mouse),LV_OBJ_FLAG_HIDDEN));
    // Inject failed writes and publication; the saved configuration must survive.
    save("home.power=calendar");
    sim_ffat_fail_write=true;
    CHECK(layout_import_sd()==LayoutResult::WriteFailed&&layout_config().home[5]==HomeTarget::Timer);
    sim_ffat_fail_publish=true;
    CHECK(layout_import_sd()==LayoutResult::WriteFailed);
    layout_load();CHECK(layout_config().home[5]==HomeTarget::Timer);
    // Export current and canonical default independently. Preserve prior SD copy.
    CHECK(layout_export_sd(false)==LayoutResult::Ok);
    CHECK(*sim_sd_files[LAYOUT_PATH]=="home.power=timer\ntools.order=Time");
    CHECK(*sim_sd_files["/Settings/layout_export.bak"]=="home.power=calendar");
    CHECK(layout_export_sd(true)==LayoutResult::Ok);
    size_t len;const auto* embedded=layout_embedded_data(len);
    CHECK(*sim_sd_files[LAYOUT_DEFAULT_PATH]==std::string(embedded,len));
    // SD absence and USB ownership cannot affect internal boot; transfer is refused.
    sim::hardware.sd_ready=false;storage_media_removed();layout_load();
    CHECK(layout_status().source==LayoutSource::Internal&&layout_config().home[5]==HomeTarget::Timer);
    CHECK(layout_import_sd()==LayoutResult::NoSd&&layout_export_sd(false)==LayoutResult::NoSd);
    sim::hardware.sd_ready=true;storage_media_inserted();
    CHECK(storage_try_usb_claim());
    CHECK(layout_import_sd()==LayoutResult::NoSd&&layout_export_sd(true)==LayoutResult::NoSd);
    storage_usb_release();
    storage_media_inserted(); // USB ejected and local FAT remounted
    // Remote source is held in NVS, never the SD boot source.
    CHECK(!layout_remote_valid_url("ftp://host/layout.txt"));
    CHECK(!layout_remote_valid_url("https://user:pass@host/layout.txt"));
    CHECK(!layout_remote_valid_url("https://"));
    CHECK(!layout_remote_valid_url("https://host/layout.txt#fragment"));
    CHECK(!layout_remote_valid_url("https://host/layout file.txt"));
    CHECK(layout_remote_valid_url("http://192.168.1.10/layout.txt"));
    settings_screen_show();pump();
    auto* url_button=label(lv_screen_active(),"Layout Source URL");CHECK(url_button);
    lv_obj_send_event(lv_obj_get_parent(url_button),LV_EVENT_CLICKED,nullptr);
    auto* url_input=textarea(lv_screen_active());CHECK(url_input);
    auto* save_url=label(lv_screen_active(),"Save URL");CHECK(save_url);
    lv_textarea_set_text(url_input,"ftp://host/layout.txt");
    lv_obj_send_event(lv_obj_get_parent(save_url),LV_EVENT_CLICKED,nullptr);
    CHECK(!*layout_remote_url());
    lv_textarea_set_text(url_input,"https://example.com/layout.txt");
    lv_obj_send_event(lv_obj_get_parent(save_url),LV_EVENT_CLICKED,nullptr);
    CHECK(!strcmp(layout_remote_url(),"https://example.com/layout.txt"));
    layout_remote_load();
    CHECK(!strcmp(layout_remote_url(),"https://example.com/layout.txt"));
    WiFi.disconnect();
    CHECK(!layout_remote_start(true)&&layout_remote_result()==LayoutRemoteResult::NoWifi);
    WiFi.begin("SIM");
    CHECK(radio_job_begin(RadioJob::Weather));
    CHECK(!layout_remote_start(true)&&layout_remote_result()==LayoutRemoteResult::RadioBusy);
    radio_job_end(RadioJob::Weather);
    // The actual Settings actions start a deferred worker. Only the UI poll
    // publishes the newly persisted layout.
    settings_screen_show();pump();
    auto* remote=label(lv_screen_active(),"Download layout to SD");CHECK(remote);
    auto* apply=label(lv_screen_active(),"Download & Apply Layout");CHECK(apply);
    layout_remote_sim_response("home.power=alarm");
    lv_obj_send_event(lv_obj_get_parent(remote),LV_EVENT_CLICKED,nullptr);
    CHECK(layout_remote_busy()&&layout_remote_result()==LayoutRemoteResult::Pending);
    CHECK(!layout_remote_start(true)&&!layout_remote_set_url("http://other/layout.txt"));
    CHECK(lv_obj_has_state(lv_obj_get_parent(apply),LV_STATE_DISABLED));
    pump();
    CHECK(layout_remote_result()==LayoutRemoteResult::Downloaded);
    CHECK(*sim_sd_files[LAYOUT_PATH]=="home.power=alarm");
    CHECK(layout_config().home[5]==HomeTarget::Timer);
    layout_remote_sim_response("home.power=stopwatch");
    lv_obj_send_event(lv_obj_get_parent(apply),LV_EVENT_CLICKED,nullptr);
    layout_remote_sim_pump();
    CHECK(layout_config().home[5]==HomeTarget::Timer);
    pump();
    CHECK(layout_remote_result()==LayoutRemoteResult::Applied);
    CHECK(layout_config().home[5]==HomeTarget::Stopwatch);
    layout_load();CHECK(layout_config().home[5]==HomeTarget::Stopwatch);
    // Failure paths: neither an invalid HTTP response nor an oversized,
    // truncated or invalid file may replace the SD editing copy or FFat.
    auto remote_fail=[&](const char* body,int response,bool complete,LayoutRemoteResult expect){
        layout_remote_sim_response(body,response,complete);
        if(!layout_remote_start(true))return false;
        pump();
        return layout_remote_result()==expect &&
            layout_config().home[5]==HomeTarget::Stopwatch &&
            *sim_sd_files[LAYOUT_PATH]=="home.power=stopwatch";
    };
    CHECK(remote_fail("home.power=alarm",302,true,LayoutRemoteResult::HttpError));
    CHECK(remote_fail("home.power=alarm",200,false,LayoutRemoteResult::Incomplete));
    CHECK(remote_fail("home.power=bad",200,true,LayoutRemoteResult::InvalidLayout));
    CHECK(remote_fail("",200,true,LayoutRemoteResult::Incomplete));
    CHECK(remote_fail(huge.c_str(),200,true,LayoutRemoteResult::TooLarge));
    layout_remote_sim_error(LayoutRemoteResult::Timeout);
    CHECK(layout_remote_start(true));pump();
    CHECK(layout_remote_result()==LayoutRemoteResult::Timeout);
    layout_remote_sim_response("home.power=alarm");
    sim::hardware.sd_ready=false;storage_media_removed();
    CHECK(!layout_remote_start(true)&&layout_remote_result()==LayoutRemoteResult::NoSd);
    sim::hardware.sd_ready=true;storage_media_inserted();
    CHECK(layout_remote_start(true));
    radio_manager_suspend(true);pump();radio_manager_suspend(false);
    CHECK(layout_remote_result()==LayoutRemoteResult::RadioBusy);
    CHECK(layout_remote_start(true));
    CHECK(storage_try_usb_claim());pump();
    CHECK(layout_remote_result()==LayoutRemoteResult::NoSd);
    storage_usb_release();storage_media_inserted();
    CHECK(layout_config().home[5]==HomeTarget::Stopwatch);
    sim_ffat_fail_write=true;
    CHECK(layout_remote_start(true));pump();
    CHECK(layout_remote_result()==LayoutRemoteResult::FlashWriteFailed);
    CHECK(layout_config().home[5]==HomeTarget::Stopwatch);
    CHECK(*sim_sd_files[LAYOUT_PATH]=="home.power=alarm");
    // A failed internal write leaves the validated download on SD for retry.
    CHECK(layout_import_sd()==LayoutResult::Ok&&layout_config().home[5]==HomeTarget::Alarm);
    deepsleep();CHECK(power_is_asleep());power_button_down(20000);
    power_tick(25000);power_button_up(25100);
    CHECK(!strcmp(layout_remote_url(),"https://example.com/layout.txt"));
    CHECK(layout_config().home[5]==HomeTarget::Alarm);
    CHECK(layout_remote_set_url(""));
    layout_remote_load();CHECK(!*layout_remote_url());
    // Simulate interrupted publication/corrupted internal primary; fallback to backup.
    auto recovered=FFat.open("/Settings/layout.bak",FILE_WRITE);
    recovered.print("home.power=calendar");recovered.close();
    auto corrupt=FFat.open(LAYOUT_PATH,FILE_WRITE);corrupt.print("home.power=bad");corrupt.close();
    layout_load();CHECK(layout_status().source==LayoutSource::Backup&&layout_status().invalid_internal);
    CHECK(layout_config().home[5]==HomeTarget::Calendar);
    CHECK(layout_restore_defaults()==LayoutResult::Ok);
    layout_load();CHECK(layout_status().source==LayoutSource::Embedded&&layout_config().home[5]==HomeTarget::Gps);
    CHECK(!FFat.exists(LAYOUT_PATH)&&!FFat.exists("/Settings/layout.bak"));
    // Mount failure is non-destructive. Only explicit initialize can format.
    sim_ffat_files["/keep.txt"]=std::make_shared<std::string>("preserve");
    sim_ffat_available=false;layout_load();
    CHECK(!layout_status().flash_ready&&layout_status().source==LayoutSource::Embedded);
    CHECK(sim_ffat_files.count("/keep.txt"));
    CHECK(layout_save_text(custom,strlen(custom))==LayoutResult::FlashUnavailable);
    CHECK(layout_initialize_flash()&&layout_status().flash_ready);
    CHECK(!sim_ffat_files.count("/keep.txt"));
    return 0;
}
