#pragma once
#include <cstdint>
#include <cstring>
#include <string>
#include <algorithm>
using ReturnCode=int;
using rfalNfcState=int;
constexpr int ST_ERR_NONE=0,RFAL_NFC_STATE_ACTIVATED=1,RFAL_NFC_POLL_TECH_A=1,RFAL_NFCDEP_GB_MAX_LEN=48;
struct rfalNfcDiscoverParam {int devLimit,techs2Find,GBLen;void(*notifyCb)(rfalNfcState);unsigned totalDuration;bool wakeupEnabled;};
struct rfalNfcDevice {uint8_t nfcidLen=4,nfcid[10]={2,0,0,1};};
struct SimNfc {
 void(*notify)(rfalNfcState)=nullptr;bool pending=false;
 rfalNfcDevice device;
 int rfalNfcDiscover(rfalNfcDiscoverParam* p){notify=p->notifyCb;pending=true;return 0;}
 void rfalNfcWorker(){if(pending&&notify){pending=false;notify(RFAL_NFC_STATE_ACTIVATED);}}
 int rfalNfcGetActiveDevice(rfalNfcDevice** p){*p=&device;return 0;}
 int rfalNfcDeactivate(bool){pending=false;return 0;}
 void rfalNfcaPollerSleep(){}
 int rfalT2TPollerRead(int page,uint8_t* out,int size,uint16_t* received){if(size<16)return 1;for(int i=0;i<16;++i)out[i]=uint8_t(page*4+i);*received=16;return 0;}
};
inline SimNfc NFCReader;
struct ndefConstBuffer {const uint8_t* buffer;uint32_t length;};
struct ndefConstBuffer8 {const uint8_t* buffer;uint8_t length;};
enum {NDEF_TYPE_RTD_TEXT,NDEF_TYPE_RTD_URI,NDEF_TYPE_MEDIA_WIFI,NDEF_TYPE_RTD_AAR};
enum {NDEF_URI_PREFIX_NONE,NDEF_URI_PREFIX_HTTP_WWW,NDEF_URI_PREFIX_HTTPS_WWW,NDEF_URI_PREFIX_HTTP,NDEF_URI_PREFIX_HTTPS,NDEF_URI_PREFIX_TEL};
constexpr int TEXT_ENCODING_UTF8=0;
struct ndefInfo {};
struct ndefType {int id=NDEF_TYPE_RTD_TEXT;std::string value="SIM NFC tag";int prefix=0;};
struct ndefRecord {ndefType type;};
struct ndefMessage {ndefRecord record;bool has_record=false;};
struct ndefTypeWifi {ndefConstBuffer bufNetworkSSID,bufNetworkKey;};
inline ndefType sim_ndef;
inline ndefRecord* ndefMessageGetFirstRecord(ndefMessage* m){return m->has_record?&m->record:nullptr;}
inline ndefRecord* ndefMessageGetNextRecord(ndefRecord*){return nullptr;}
class NdefClass {
public:
 explicit NdefClass(SimNfc*){}
 int ndefPollerContextInitialization(rfalNfcDevice*){return 0;}
 int ndefPollerNdefDetect(ndefInfo*){return 0;}
 int ndefPollerReadRawMessage(uint8_t* out,uint32_t size,uint32_t* actual){*actual=std::min(size,uint32_t(sim_ndef.value.size()));std::memcpy(out,sim_ndef.value.data(),*actual);return 0;}
 int ndefMessageDecode(ndefConstBuffer*,ndefMessage* m){m->record.type=sim_ndef;m->has_record=true;return 0;}
 int ndefRecordToType(ndefRecord* r,ndefType* t){*t=r->type;return 0;}
 int ndefGetRtdText(ndefType* t,uint8_t* enc,ndefConstBuffer8* lang,ndefConstBuffer* text){*enc=0;*lang={(const uint8_t*)"en",2};*text={(const uint8_t*)t->value.data(),uint32_t(t->value.size())};return 0;}
 int ndefGetRtdUri(ndefType* t,ndefConstBuffer* prefix,ndefConstBuffer* url){static const char* p[]={"","http://www.","https://www.","http://","https://","tel:"};const char* v=p[t->prefix%6];*prefix={(const uint8_t*)v,uint32_t(std::strlen(v))};*url={(const uint8_t*)t->value.data(),uint32_t(t->value.size())};return 0;}
 int ndefGetWifi(ndefType*,ndefTypeWifi* w){*w={{(const uint8_t*)"SIM",3},{(const uint8_t*)"SIM-KEY",7}};return 0;}
 int ndefGetRtdAar(ndefType*,ndefConstBuffer* b){*b={(const uint8_t*)"sim.app",7};return 0;}
 int ndefPollerTagFormat(void*,int){return 0;}
 int ndefRtdUri(ndefType* t,int prefix,const ndefConstBuffer* v){t->id=NDEF_TYPE_RTD_URI;t->prefix=prefix;t->value.assign((const char*)v->buffer,v->length);return 0;}
 int ndefRtdText(ndefType* t,int,ndefConstBuffer8*,ndefConstBuffer* v){t->id=NDEF_TYPE_RTD_TEXT;t->value.assign((const char*)v->buffer,v->length);return 0;}
 int ndefTypeToRecord(ndefType* t,ndefRecord* r){r->type=*t;return 0;}
 void ndefMessageInit(ndefMessage* m){*m={};}
 void ndefMessageAppend(ndefMessage* m,ndefRecord* r){m->record=*r;m->has_record=true;}
 int ndefPollerWriteMessage(ndefMessage* m){sim_ndef=m->record.type;return 0;}
};
