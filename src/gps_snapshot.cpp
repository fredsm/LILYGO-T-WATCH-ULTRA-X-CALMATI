#include "gps_snapshot.h"
#ifdef TWATCH_SIMULATOR
#include <mutex>
#else
#include <freertos/FreeRTOS.h>
#endif
namespace {
#ifdef TWATCH_SIMULATOR
std::mutex mux;
struct Guard{std::lock_guard<std::mutex> lock{mux};};
#else
portMUX_TYPE mux=portMUX_INITIALIZER_UNLOCKED;
struct Guard{Guard(){portENTER_CRITICAL(&mux);}~Guard(){portEXIT_CRITICAL(&mux);}};
#endif
uint32_t current=0;
bool active=false;
GpsSnapshot latest;
}
void gps_snapshot_session(uint32_t gen,bool enabled){
    Guard g;current=gen;active=enabled;latest=GpsSnapshot{};
}
bool gps_snapshot_publish(uint32_t gen,const GpsSnapshot& fix){
    Guard g;if(!active || gen!=current)return false;latest=fix;return true;
}
GpsSnapshot gps_snapshot(){Guard g;return latest;}
