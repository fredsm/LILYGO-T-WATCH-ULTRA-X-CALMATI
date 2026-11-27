#pragma once
#include <stdint.h>
#include <string.h>
// Canonical 16 kHz, 16-bit mono PCM, matching the fixed PDM format.
inline void pcm_wav_header(uint8_t* h,uint32_t bytes){
    auto u32=[](uint8_t* p,uint32_t v){p[0]=v;p[1]=v>>8;p[2]=v>>16;p[3]=v>>24;};
    auto u16=[](uint8_t* p,uint16_t v){p[0]=v;p[1]=v>>8;};
    memcpy(h,"RIFF",4);u32(h+4,36+bytes);
    memcpy(h+8,"WAVEfmt ",8);u32(h+16,16);
    u16(h+20,1);u16(h+22,1);u32(h+24,16000);u32(h+28,32000);
    u16(h+32,2);u16(h+34,16);
    memcpy(h+36,"data",4);u32(h+40,bytes);
}
