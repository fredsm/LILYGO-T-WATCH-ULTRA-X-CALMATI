#include "audio_manager.h"
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
AudioStats stats;
uint32_t serial=0,mic_token=0,speaker_token=0;
bool is_mic(AudioClient c){return c==AudioClient::Recorder || c==AudioClient::WakeWord;}
}
uint32_t audio_acquire(AudioClient c){
    Guard g;
    auto& owner=is_mic(c)?stats.mic:stats.speaker;
    auto& token=is_mic(c)?mic_token:speaker_token;
    if((!is_mic(c)&&c!=AudioClient::Chime)||stats.suspended||owner!=AudioClient::None){
        ++stats.rejected;return 0;
    }
    if(++serial==0)++serial;
    owner=c;token=serial;++stats.sessions;return token;
}
void audio_release(AudioClient c,uint32_t t){
    Guard g;
    auto& owner=is_mic(c)?stats.mic:stats.speaker;
    auto& token=is_mic(c)?mic_token:speaker_token;
    if(t && owner==c && t==token){owner=AudioClient::None;token=0;++stats.completed;}
}
void audio_suspend(bool suspend){Guard g;stats.suspended=suspend;}
bool audio_idle(){Guard g;return stats.mic==AudioClient::None && stats.speaker==AudioClient::None;}
AudioStats audio_stats(){Guard g;return stats;}
const char* audio_client_name(AudioClient c){
    switch(c){case AudioClient::Recorder:return "Recorder";case AudioClient::WakeWord:return "Wake word";case AudioClient::Chime:return "Chime";default:return "Idle";}
}
void audio_note_io(bool read,bool ok,uint32_t us){
    Guard g;
    auto& maximum=read?stats.max_read_us:stats.max_write_us;
    if(us>maximum)maximum=us;
    if(!ok)++(read?stats.read_errors:stats.write_errors);
}
void audio_note_buffer(uint32_t queued,uint32_t dropped){
    Guard g;if(queued>stats.buffer_peak)stats.buffer_peak=queued;stats.dropped_bytes+=dropped;
}
void audio_note_sd(uint32_t us){Guard g;if(us>stats.max_sd_us)stats.max_sd_us=us;}
