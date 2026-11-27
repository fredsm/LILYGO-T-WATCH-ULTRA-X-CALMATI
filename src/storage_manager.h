#pragma once
#include <stddef.h>
#include <stdint.h>

// Bounded, FIFO SD work queue. All accepted payloads are immutable, owned by
// the manager until completion. Workers must never call LVGL or I2C.
enum class StorageMode : uint8_t { Replace, Append };
using StorageJobFn = bool (*)(const char* path, const void* data, size_t bytes);
struct StorageStats {
    uint32_t accepted=0, completed=0, failed=0, rejected=0, cancelled=0, coalesced=0;
    uint32_t max_job_ms=0, queued_bytes=0;
    unsigned pending=0, local_users=0;
    bool active=false, media=false, usb_owned=false, accepting=false, worker=false;
};
void storage_init(bool media);
void* storage_allocate(size_t bytes); // free() compatible (large buffers use PSRAM)
bool storage_submit_owned(const char* path, void* data, size_t bytes, StorageJobFn fn);
bool storage_submit_write(const char* path, const void* data, size_t bytes, StorageMode mode);
bool storage_submit_owned_write(const char* path, void* data, size_t bytes, StorageMode mode);
// Owned submission consumes data on BOTH success and rejection.
StorageStats storage_stats();
struct StorageUsage { uint64_t total=0, used=0; bool valid=false, pending=false; };
bool storage_request_usage();
StorageUsage storage_usage();

// Existing synchronous readers/writers take a short lifecycle lease (not a
// filesystem mutex). This prevents unmount/MSC takeover while a File is live.
// The native FAT and SPI drivers still arbitrate individual transactions.
bool storage_local_acquire();
void storage_local_release();
class StorageAccess {
    bool held_;
public:
    StorageAccess():held_(storage_local_acquire()){}
    ~StorageAccess(){if(held_)storage_local_release();}
    explicit operator bool() const{return held_;}
    StorageAccess(const StorageAccess&)=delete;
    StorageAccess& operator=(const StorageAccess&)=delete;
};
bool storage_try_usb_claim();
void storage_usb_release();
bool storage_usb_acquire();
void storage_usb_io_release();
bool storage_usb_owned();
bool storage_needs_remount(); // host may have modified FAT; local gate stays closed
void storage_media_removed(); // stop accepting; queued jobs are cancelled
void storage_media_inserted();
bool storage_idle();
void storage_suspend();       // stop accepting; drain accepted jobs
void storage_resume();
void storage_release_image_cache();
void storage_bind_lvgl();     // gate POSIX LVGL file/dir lifetimes on drive A

#ifdef TWATCH_SIMULATOR
// Deterministic backend: normally completes inline; tests may defer/pump.
void storage_sim_defer(bool defer);
bool storage_sim_pump();
#endif
