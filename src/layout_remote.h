#pragma once
#include <stddef.h>
#include <stdint.h>

constexpr size_t LAYOUT_URL_MAX=512;
enum class LayoutRemoteResult:uint8_t {
    Idle, Pending, Downloaded, Applied, InvalidUrl, SaveFailed, NoWifi,
    RadioBusy, NoSd, HttpError, TooLarge, Incomplete, Timeout, TlsUnavailable,
    InvalidLayout, WriteFailed, FlashWriteFailed, FlashUnavailable, Cancelled, WorkerFailed
};
bool layout_remote_valid_url(const char* url);
const char* layout_remote_url();
bool layout_remote_set_url(const char* url); // empty clears; NVS, not SD
void layout_remote_load();
bool layout_remote_busy();
LayoutRemoteResult layout_remote_result();
const char* layout_remote_message();
bool layout_remote_start(bool activate);
bool layout_remote_poll(); // UI owner; true when a completed result is applied
#ifdef TWATCH_SIMULATOR
// No real network from simulator. Inject a deterministic HTTP response.
void layout_remote_sim_response(const char* body, int status=200,bool complete=true);
void layout_remote_sim_error(LayoutRemoteResult error);
void layout_remote_sim_pump();
#endif
