#include "SD.h"
#include <lvgl.h>
namespace {
const uint8_t tile[]={
#include "map_tile.inc"
};
void* open_file(lv_fs_drv_t*,const char* path,lv_fs_mode_t mode){
 if(mode!=LV_FS_MODE_RD)return nullptr;
 std::string key=path;if(key.empty()||key[0]!='/')key='/'+key;
 if(key.find("/map/")==0 && key.size()>4 && key.substr(key.size()-4)==".png" && !SD.exists(key.c_str())){
  auto f=SD.open(key.c_str(),FILE_WRITE);f.write(tile,sizeof(tile));
 }
 auto f=SD.open(key.c_str());if(!f)return nullptr;return new File(f);
}
lv_fs_res_t close_file(lv_fs_drv_t*,void* file){delete static_cast<File*>(file);return LV_FS_RES_OK;}
lv_fs_res_t read_file(lv_fs_drv_t*,void* file,void* out,uint32_t n,uint32_t* got){*got=static_cast<File*>(file)->read((uint8_t*)out,n);return LV_FS_RES_OK;}
lv_fs_res_t seek_file(lv_fs_drv_t*,void* file,uint32_t pos,lv_fs_whence_t where){auto* f=static_cast<File*>(file);size_t base=where==LV_FS_SEEK_CUR?f->position():where==LV_FS_SEEK_END?f->size():0;return f->seek(base+pos)?LV_FS_RES_OK:LV_FS_RES_FS_ERR;}
lv_fs_res_t tell_file(lv_fs_drv_t*,void* file,uint32_t* pos){*pos=uint32_t(static_cast<File*>(file)->position());return LV_FS_RES_OK;}
}
namespace sim {
void initialize_filesystem(){
 SD.mkdir("/map");SD.mkdir("/map/1");SD.mkdir("/map/2");
 auto fix=SD.open("/Meshtastic/last_fix.txt",FILE_WRITE);fix.print("35.0,-80.0\n");
 static lv_fs_drv_t driver;lv_fs_drv_init(&driver);driver.letter='A';
 driver.open_cb=open_file;driver.close_cb=close_file;driver.read_cb=read_file;driver.seek_cb=seek_file;driver.tell_cb=tell_file;lv_fs_drv_register(&driver);
}
}
