#include "../../src/audio_manager.h"
#include "../../src/gps_snapshot.h"
#include "../../src/pcm_wav.h"
#include <cstdlib>
#include <thread>
#define CHECK(x) do { if(!(x)) std::abort(); } while(0)
int main(){
    uint8_t h[44];
    auto word=[&](int off){return uint32_t(h[off]) | uint32_t(h[off+1])<<8 |
        uint32_t(h[off+2])<<16 | uint32_t(h[off+3])<<24;};
    pcm_wav_header(h,0);
    CHECK(!memcmp(h,"RIFF",4) && word(4)==36 && word(40)==0);
    pcm_wav_header(h,32000);
    CHECK(word(4)==32036 && word(40)==32000 && word(24)==16000 && word(28)==32000);
    CHECK(!memcmp(h+8,"WAVEfmt ",8) && !memcmp(h+36,"data",4) && h[20]==1 && h[22]==1 && h[32]==2 && h[34]==16);
    CHECK(audio_idle());
    auto recorder=audio_acquire(AudioClient::Recorder);
    CHECK(recorder && !audio_idle());
    auto speaker=audio_acquire(AudioClient::Chime);
    CHECK(speaker && speaker!=recorder);
    std::thread other([]{CHECK(!audio_acquire(AudioClient::WakeWord));CHECK(!audio_acquire(AudioClient::Chime));});
    other.join();
    audio_release(AudioClient::WakeWord,recorder);CHECK(!audio_idle());
    audio_suspend(true);
    audio_release(AudioClient::Recorder,recorder);
    CHECK(!audio_acquire(AudioClient::Recorder));
    audio_release(AudioClient::Chime,speaker);
    CHECK(audio_idle());
    audio_suspend(false);
    auto wake=audio_acquire(AudioClient::WakeWord);
    CHECK(wake && wake!=recorder);
    audio_release(AudioClient::WakeWord,recorder);
    CHECK(audio_stats().mic==AudioClient::WakeWord);
    audio_note_io(true,true,150);
    audio_note_io(true,false,200);
    audio_note_io(false,false,80);
    audio_note_buffer(4096,0);audio_note_buffer(1024,512);
    audio_note_sd(12000);
    auto a=audio_stats();
    CHECK(a.sessions==3 && a.completed==2 && a.rejected==3);
    CHECK(a.read_errors==1 && a.write_errors==1 && a.max_read_us==200);
    CHECK(a.buffer_peak==4096 && a.dropped_bytes==512 && a.max_sd_us==12000);
    audio_release(AudioClient::WakeWord,wake);CHECK(audio_idle());

    gps_snapshot_session(1,true);
    GpsSnapshot f;f.valid=true;f.lat=35.5;f.lon=-80.5;f.fix_ms=UINT32_MAX-20;
    CHECK(gps_snapshot_publish(1,f));
    CHECK(gps_snapshot().fresh(10,32) && !gps_snapshot().fresh(10,31));
    gps_snapshot_session(2,false);
    CHECK(!gps_snapshot().valid && !gps_snapshot_publish(1,f));
    gps_snapshot_session(3,true);
    CHECK(!gps_snapshot_publish(1,f) && !gps_snapshot().valid);
    CHECK(gps_snapshot_publish(3,f) && gps_snapshot().lat==35.5);
    std::thread reader([]{for(int i=0;i<1000;++i){auto v=gps_snapshot();CHECK(!v.valid || (v.lat==35.5 && v.lon==-80.5));}});
    for(int i=0;i<1000;++i){gps_snapshot_session(3,true);CHECK(gps_snapshot_publish(3,f));}
    reader.join();
}
