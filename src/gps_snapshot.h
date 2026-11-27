#pragma once
#include <stdint.h>
struct GpsSnapshot {
    bool valid=false;
    double lat=0,lon=0;
    uint32_t fix_ms=0;
    bool fresh(uint32_t now,uint32_t max_age) const {return valid && uint32_t(now-fix_ms)<max_age;}
};
void gps_snapshot_session(uint32_t generation,bool enabled);
bool gps_snapshot_publish(uint32_t generation,const GpsSnapshot& fix);
GpsSnapshot gps_snapshot();
