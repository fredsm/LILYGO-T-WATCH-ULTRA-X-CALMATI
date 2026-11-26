#pragma once
#include "watch.hpp"
namespace sim {
// Shared, volatile fixtures. No host devices or network connections are used.
struct Hardware {
    Watch watch;
    bool sd_ready=true, gps_fix=true;
    int satellites=8;
    Hardware(){watch.set_battery(75);}
    void reset_inputs(){
        watch.set_battery(75);watch.set_charging(false);
        watch.set_brightness(255);watch.set_display_on(true);
        sd_ready=true;gps_fix=true;satellites=8;
    }
};
inline Hardware hardware;
}
