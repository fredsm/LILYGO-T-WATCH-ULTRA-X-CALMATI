#include "../../src/storage_manager.h"
#include "../../src/storage_text.h"
#include "mock/SD.h"
#include <cstdio>
#include <cstring>
#include <lvgl.h>
namespace sim {void initialize_filesystem();}
#define CHECK(e) do{if(!(e)){std::fprintf(stderr,"line %d: %s\n",__LINE__,#e);return 1;}}while(0)
int main(){
    sim::hardware.sd_ready=true;
    storage_init(true);storage_sim_defer(true);
    char input[]="one";
    CHECK(storage_submit_write("/Test/a",input,3,StorageMode::Replace));
    input[0]='X';
    CHECK(storage_submit_write("/Test/a","two",3,StorageMode::Append));
    CHECK(!storage_try_usb_claim());
    CHECK(storage_stats().pending==2&&storage_stats().queued_bytes==6);
    CHECK(storage_sim_pump()&&storage_sim_pump()&&!storage_sim_pump());
    CHECK(*sim_sd_files["/Test/a"]=="onetwo");
    CHECK(storage_submit_write("/Settings/a","old",3,StorageMode::Replace));
    CHECK(storage_submit_write("/Settings/a","new",3,StorageMode::Replace));
    CHECK(storage_stats().coalesced==1&&storage_stats().pending==1);
    CHECK(storage_sim_pump()&&*sim_sd_files["/Settings/a"]=="new");

    // Queue limits, reserved settings capacity, and ownership transfer.
    for(unsigned i=0;i<10;++i)CHECK(storage_submit_write("/Test/b","x",1,StorageMode::Append));
    CHECK(!storage_submit_write("/Test/b","x",1,StorageMode::Append));
    CHECK(storage_submit_write("/Settings/a","now",3,StorageMode::Replace));
    CHECK(storage_submit_write("/Settings/b","yes",3,StorageMode::Replace));
    CHECK(!storage_submit_write("/Settings/c","no",2,StorageMode::Replace));
    storage_suspend();
    CHECK(!storage_submit_write("/Test/c","x",1,StorageMode::Replace));
    CHECK(!storage_idle());
    while(storage_sim_pump()){}
    CHECK(storage_idle());
    storage_resume();
    CHECK(*sim_sd_files["/Test/b"]=="xxxxxxxxxx");

    // The POSIX/LVGL adapter protects cached image or other live files.
    lv_init();sim::initialize_filesystem();storage_bind_lvgl();
    lv_fs_file_t file{};
    CHECK(lv_fs_open(&file,"A:/Test/a",LV_FS_MODE_RD)==LV_FS_RES_OK);
    CHECK(!storage_try_usb_claim());
    CHECK(lv_fs_close(&file)==LV_FS_RES_OK);
    // A live local file lease blocks host takeover. A raw USB operation
    // blocks release until its last callback is finished.
    {
        StorageAccess lease;CHECK(bool(lease));CHECK(!storage_try_usb_claim());
    }
    CHECK(storage_try_usb_claim());
    CHECK(!storage_local_acquire()&&!storage_submit_write("/Test/a","x",1,StorageMode::Append));
    CHECK(lv_fs_open(&file,"A:/Test/a",LV_FS_MODE_RD)!=LV_FS_RES_OK);
    CHECK(storage_usb_acquire());
    storage_usb_release();
    CHECK(storage_usb_owned()&&!storage_idle()&&!storage_usb_acquire());
    storage_usb_io_release();
    CHECK(!storage_usb_owned()&&storage_idle()&&storage_needs_remount());
    CHECK(!storage_local_acquire()&&!storage_try_usb_claim());
    storage_media_inserted(); // SDK remount completed

    // Removal cancels queued writes rather than sending them to a new card.
    CHECK(storage_submit_write("/Test/a","lost",4,StorageMode::Replace));
    storage_media_removed();
    CHECK(!storage_stats().accepting);
    CHECK(storage_sim_pump()&&storage_stats().cancelled==1);
    CHECK(*sim_sd_files["/Test/a"]=="onetwo");
    storage_media_inserted();
    CHECK(storage_request_usage()&&storage_request_usage());
    CHECK(storage_stats().pending==1&&storage_usage().pending);
    CHECK(storage_sim_pump()&&storage_usage().valid&&!storage_usage().pending);
    CHECK(storage_usage().total==SD.totalBytes());
    CHECK(storage_submit_write("/Test/fail","x",1,StorageMode::Replace));
    sim::hardware.sd_ready=false;
    CHECK(storage_sim_pump()&&storage_stats().failed==1);
    sim::hardware.sd_ready=true;
    StorageText text;text.printf("%s %d\n","sample",123);
    CHECK(text.commit("/Test/text",StorageMode::Replace)&&storage_sim_pump());
    CHECK(*sim_sd_files["/Test/text"]=="sample 123\n");
    StorageText overflow;char big[2050];memset(big,'a',sizeof(big)-1);big[sizeof(big)-1]=0;
    overflow.print(big);
    CHECK(!overflow.commit("/Test/overflow",StorageMode::Replace));
    CHECK(!SD.exists("/Test/overflow"));
    auto s=storage_stats();
    CHECK(s.accepted==s.completed+s.failed+s.cancelled+s.coalesced);
    return 0;
}
