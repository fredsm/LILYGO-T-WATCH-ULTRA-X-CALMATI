#pragma once
#include <stdint.h>

// Separate I2S controllers: one capture owner and one playback owner.
// A session token prevents a late completion releasing a newer owner.
enum class AudioClient : uint8_t { None, Recorder, WakeWord, Chime };
struct AudioStats {
    AudioClient mic=AudioClient::None, speaker=AudioClient::None;
    bool suspended=false;
    uint32_t rejected=0, sessions=0, completed=0;
    uint32_t read_errors=0, write_errors=0, dropped_bytes=0;
    uint32_t max_read_us=0, max_write_us=0, buffer_peak=0, max_sd_us=0;
};
uint32_t audio_acquire(AudioClient client);
void audio_release(AudioClient client,uint32_t token);
void audio_suspend(bool suspend);
bool audio_idle();
AudioStats audio_stats();
const char* audio_client_name(AudioClient client);
void audio_note_io(bool read,bool ok,uint32_t us);
void audio_note_buffer(uint32_t queued,uint32_t dropped);
void audio_note_sd(uint32_t us);
