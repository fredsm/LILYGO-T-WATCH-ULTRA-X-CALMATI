#include "../../src/sim/screens.hpp"
#include <array>
#include <cstdio>
#include <cstring>
namespace {
std::array<uint16_t,410*32> buffer{};
unsigned flushes=0;
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){++flushes;lv_display_flush_ready(d);}
void pump(){for(int i=0;i<150;++i){lv_tick_inc(20);lv_timer_handler();}}
}
int main(int argc,char** argv){
 if(argc!=2)return 1;
 lv_init();auto* display=lv_display_create(410,502);
 lv_display_set_color_format(display,LV_COLOR_FORMAT_RGB565);
 lv_display_set_buffers(display,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
 lv_display_set_flush_cb(display,flush);
 auto* home=lv_screen_active();sim::initialize_screens(home);
 std::size_t count=0;const auto* entries=sim::screens(count);int tested=0;
 for(std::size_t i=0;i<count;++i){
  const auto& s=entries[i];if(std::strcmp(argv[1],"--all") && std::strcmp(argv[1],s.name))continue;
  std::printf("Opening %s\n",s.name);std::fflush(stdout);
  for(int pass=0;pass<2;++pass){
   unsigned before=flushes;s.show();pump();
   if(!s.active() || !lv_obj_get_child_count(lv_screen_active()) || flushes==before)return 2;
   sim::show_home();pump();if(lv_screen_active()!=home)return 3;
  }
  ++tested;
 }
 lv_deinit();std::printf("Verified %d screens, %u flushes\n",tested,flushes);return tested?0:4;
}
