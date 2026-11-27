#pragma once
#include "watch.hpp"
namespace sim {
// Shared, volatile fixtures. No host devices or network connections are used.
struct Hardware {
    Watch watch;
    bool sd_ready=true, gps_fix=true;
    int satellites=8;
    unsigned core_load[2]={10,10}; // injected demand, not host CPU utilization
    Hardware(){watch.set_battery(75);}
    void reset_inputs(){
        watch.set_battery(75);watch.set_charging(false);
        watch.set_brightness(255);watch.set_display_on(true);
        sd_ready=true;gps_fix=true;satellites=8;core_load[0]=core_load[1]=10;
    }
};
inline Hardware hardware;
}
