#include "../../src/radio_manager.h"
#include <cstdlib>
#include <thread>
#define CHECK(x) do { if(!(x)) std::abort(); } while(0)
int main(){
    radio_manager_init();
    {
        RadioTransition a,b;
        CHECK(a && b);
        CHECK(!radio_action_allowed(RadioAction::WifiUp,false,true));
        CHECK(!radio_action_allowed(RadioAction::BleUp,true,false));
        CHECK(radio_action_allowed(RadioAction::WifiDown,true,false));
        CHECK(radio_action_allowed(RadioAction::BleUp,false,false));
        std::thread waiter([]{RadioTransition l;CHECK(!l);});
        waiter.join();
    }
    CHECK(radio_job_begin(RadioJob::Weather));
    CHECK(radio_job_busy());
    CHECK(!radio_job_begin(RadioJob::Adsb));
    CHECK(radio_action_allowed(RadioAction::WifiUp,false,false));
    CHECK(!radio_action_allowed(RadioAction::BleUp,false,false));
    std::thread other([]{
        CHECK(!radio_job_begin(RadioJob::TimeSync));
        RadioTransition l;CHECK(l);
        CHECK(!radio_action_allowed(RadioAction::WifiDown,true,false));
        CHECK(!radio_action_allowed(RadioAction::WifiChange,true,false));
        radio_job_end(RadioJob::Weather);
    });
    other.join();
    CHECK(radio_job_busy());
    radio_manager_suspend(true);
    CHECK(radio_action_allowed(RadioAction::WifiDown,true,false));
    radio_job_end(RadioJob::Weather);
    CHECK(!radio_job_busy());
    CHECK(!radio_job_begin(RadioJob::Adsb));
    CHECK(!radio_action_allowed(RadioAction::WifiUp,false,false));
    CHECK(radio_action_allowed(RadioAction::BleDown,false,true));
    radio_manager_suspend(false);
    CHECK(radio_job_begin(RadioJob::TimeSync));
    radio_job_end(RadioJob::TimeSync);
    radio_action_record(RadioAction::WifiUp,true,100);
    radio_action_record(RadioAction::WifiDown,false,125);
    radio_action_record(RadioAction::BleUp,true,200);
    auto s=radio_manager_stats();
    CHECK(s.job==RadioJob::None && s.completed==2 && s.lock_timeouts==1);
    CHECK(s.rejected>=9 && s.wifi_calls==2 && s.ble_calls==1 && s.failures==1);
    CHECK(s.wifi_max_us==125 && s.ble_max_us==200);
}
