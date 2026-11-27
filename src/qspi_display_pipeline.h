#pragma once
#include <stddef.h>
#include <stdint.h>

// One persistent transaction owned by the transport until it is reaped.
// The transport must copy payloads before accepting a queue operation.
enum class QspiReap : uint8_t { Complete, Pending, Error };
struct QspiTransport {
    void* context;
    bool (*queue)(void*, bool pixels, uint8_t command, const uint8_t*, size_t, bool swap);
    QspiReap (*reap)(void*, uint32_t wait_ms);
    uint64_t (*now_us)(void*);
};
struct QspiStats {
    bool enabled=false, paused=false, pending=false, quarantined=false, cooling=false;
    uint32_t flushes=0, completed=0, dropped=0, commands=0, errors=0, timeouts=0, recoveries=0;
    uint32_t max_wait_us=0, max_transfer_us=0, max_submit_us=0, active_us=0;
    uint64_t bytes=0;
};
class QspiPipeline {
public:
    static constexpr size_t capacity=8192;
    static constexpr uint32_t wait_ms=20;
    explicit QspiPipeline(QspiTransport transport): io_(transport) {}
    void enable();
    void pause(bool paused);
    bool flush(uint16_t x,uint16_t y,uint16_t w,uint16_t h,const uint8_t* data,bool swap=true);
    // LVGL can release its render buffer even on failure: DMA only owns a copy.
    bool wait();
    bool idle(); // zero-time poll, no wait
    bool command(uint8_t cmd,const uint8_t* data,size_t len);
    bool service(); // true once after a fault, when a full repaint is safe
    QspiStats stats() const;
private:
    bool submit(bool pixels,uint8_t cmd,const uint8_t* data,size_t len,bool swap);
    bool reap(uint32_t ms);
    bool available();
    void fault(bool timeout);
    uint64_t now() const {return io_.now_us(io_.context);}
    QspiTransport io_;
    QspiStats s_;
    uint64_t queued_at_=0,retry_at_=0;
    bool pixel_pending_=false, repaint_=false;
};
