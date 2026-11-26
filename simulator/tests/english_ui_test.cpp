#include "../../src/sim/screens.hpp"
#include "../../src/sim/clock.hpp"
#include <array>
#include <cstdio>
#include <cstring>
namespace {
std::array<uint16_t,410*32> buffer{};
void flush(lv_display_t* d,const lv_area_t*,uint8_t*){lv_display_flush_ready(d);}
bool contains(lv_obj_t* obj,const char* text){
    if(lv_obj_check_type(obj,&lv_label_class)&&std::strstr(lv_label_get_text(obj),text))return true;
    for(unsigned i=0;i<lv_obj_get_child_count(obj);++i)if(contains(lv_obj_get_child(obj,i),text))return true;
    return false;
}
}
int main(){
    lv_init();auto* d=lv_display_create(410,502);
    lv_display_set_color_format(d,LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(d,buffer.data(),nullptr,sizeof(buffer),LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(d,flush);sim::initialize_screens(sim::create_clock());
    if(!contains(sim::create_clock(),"weather")||contains(sim::create_clock(),"meteo"))return 2;
    struct Expect {const char* screen;const char* english;const char* italian;};
    const Expect checks[]={
        {"tools","NFC Compare","Credito"},{"tools","Car","Auto"},
        {"car","SAVE CAR LOCATION","SALVA POSIZIONE AUTO"},
        {"voice","SPEAK","PARLA"},{"nfc_credit","BEFORE","PRIMA"},
        {"nfc_credit","AFTER","DOPO"},{"drone","drones","droni"},
        {"gatt_explorer","GATT","Scansione BLE"},
        {"meta","META GLASSES","Scansione BLE"},{"scan","SCANNER","Aria pulita"},
        {"phantom_flood","Name:","Nome:"},{"rid_spoof","TRANSMIT","TRASMETTI"},
        {"printer","PRINT CALMATI","STAMPA CALMATI"},
        {"waterfall","Band Analyzer","Analizzatore Banda"},
        {"subghz_sentinel","Baseline","fondo"},
        {"calmati_remote","R:Stations","R:Staz"},
        {"calmati_targets","BACK","INDIETRO"},{"calmati_result","BACK","INDIETRO"},
        {"evil_twin","CLONE","CLONA"},{"wpa3_sae","FLOOD","RISCAN"},
        {"skimmer","CLOSE","CHIUDI"}
    };
    std::size_t count=0;auto* entries=sim::screens(count);
    for(const auto& check:checks){
        sim::show_home();bool found=false;
        for(std::size_t i=0;i<count;++i)if(!std::strcmp(entries[i].name,check.screen)){entries[i].show();found=true;break;}
        lv_tick_inc(40);sim::tick();lv_timer_handler();
        if(!found||!contains(lv_screen_active(),check.english)||contains(lv_screen_active(),check.italian)){
            std::fprintf(stderr,"English UI check failed: %s (%s)\n",check.screen,check.english);return 1;
        }
    }
    sim::show_home();lv_deinit();return 0;
}
