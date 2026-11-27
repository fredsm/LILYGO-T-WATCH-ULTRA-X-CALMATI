#pragma once
#include <stdint.h>

// UI-owned state machine. A poll never waits for a resource; the caller
// requests shutdown once, then checks acknowledgement on later UI ticks.
enum class PowerStage : uint8_t { Awake, ScreenOff, LightSleep, Stopping, FinalStorage, Draining, Drivers, DeepSleep };
enum PowerBlock : uint32_t {
    BlockRadio=1u<<0, BlockAudio=1u<<1, BlockStorage=1u<<2,
    BlockDisplay=1u<<3, BlockSpi=1u<<4, BlockI2c=1u<<5,
    BlockPeripheral=1u<<6, BlockDeadline=1u<<7, BlockUsb=1u<<8,
    BlockMotion=1u<<9, BlockIrq=1u<<10, BlockDriver=1u<<11
};
struct PowerLifecycleStats {
    PowerStage stage=PowerStage::Awake;
    uint32_t blockers=0, last_idle_blockers=0, shutdown_blockers=0, shutdown_ms=0,
             last_shutdown_ms=0, last_shutdown_blockers=0, light_entries=0, light_wakes=0,
             light_aborts=0, last_light_ms=0, max_drain_ms=0;
};
class PowerLifecycle {
public:
    static constexpr uint32_t idle_grace_ms=5000;
    void screen(bool off,uint32_t now);
    bool idle_due(uint32_t now) const;
    void idle_blocked(uint32_t blockers){if(stats_.stage==PowerStage::ScreenOff){stats_.blockers=blockers;stats_.last_idle_blockers=blockers;}}
    void begin_shutdown(uint32_t now);
    // True only when this stage's blockers are clear. No forced timeout.
    bool advance(uint32_t blockers,uint32_t now);
    void light_enter(uint32_t now);
    void light_exit(uint32_t now,bool slept);
    void reboot();
    PowerLifecycleStats snapshot(uint32_t now) const;
private:
    PowerLifecycleStats stats_{};
    uint32_t off_at_=0, shutdown_at_=0, light_at_=0;
    bool shutting_=false;
};
const char* power_stage_name(PowerStage stage);
