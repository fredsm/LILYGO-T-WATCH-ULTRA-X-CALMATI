#include "layout_config.h"
#include "layout_default_embedded.h"
#include <string.h>
#include <stdlib.h>

namespace {
// Order matches the existing compiled Tools menu; labels are stable config IDs.
const char* const tools[]={
    "Time","Wardriver","Scanner","Duress","WiFi","Analyze","Mouse","USB SD",
    "Pager","TPMS","LoRa APRS","Tesla CP","AirTag","Skimmers","Evil Twin","Flock",
    "Radar","Meta","Task Mgr","Pet","Pwn","Cameras","Deauth","ARP MitM",
    "BLE Audit","Drone ID","GATT","Phantom","RID Spoof","Sentinel","Printer",
    "Calmati","Download","Mifare","Waterfall","ESP-NOW","Car","UDDA","ADS-B",
    "Mic Rec","Assist","Panel","NFC Compare","WPA3 SAE"
};
static_assert(sizeof(tools)/sizeof(tools[0])==LAYOUT_TOOL_COUNT,"Tool inventory mismatch");
const char* const controls[]={"wifi","bluetooth","gps","lora","nfc","brightness"};
const char* const actions[]={"settings","lock"};
const char* const targets[]={"none","time","meshtastic","tools","quick_controls","settings","gps","lora",
    "wifi_radio","bluetooth","nfc","calendar","stopwatch","timer","alarm","diagnostics","task_manager"};
const char* const keys[]={"home.swipe_left","home.swipe_right","home.swipe_up","home.swipe_down",
    "home.back","home.power","tools.order","tools.hide","quick.controls","quick.actions",
    "available-screens","available-home-targets"};
char* trim(char* s){
    while(*s==' '||*s=='\t'||*s=='\r')++s;
    char* end=s+strlen(s);
    while(end>s&&(end[-1]==' '||end[-1]=='\t'||end[-1]=='\r'))*--end=0;
    return s;
}
template<size_t N> int find(const char* s,const char* const(&names)[N]){
    for(unsigned i=0;i<N;++i)if(!strcmp(s,names[i]))return int(i);
    return -1;
}
template<size_t N> bool list(char* s,const char* const(&names)[N],uint8_t* out,uint8_t& count){
    count=0;bool seen[N]={};
    if(!*s)return true;
    while(true){
        char* comma=strchr(s,',');if(comma)*comma=0;
        int id=find(trim(s),names);
        if(id<0||seen[id]||count>=N)return false;
        seen[id]=true;out[count++]=uint8_t(id);
        if(!comma)return true;
        s=comma+1;
    }
}
}
// Registry order is only the append order for tools absent from configuration.
// The complete standard layout is parsed from the embedded canonical file.
static LayoutConfig inventory(){
    LayoutConfig c{};
    for(unsigned i=0;i<LAYOUT_TOOL_COUNT;++i)c.tools[i]=i;
    c.tool_count=LAYOUT_TOOL_COUNT;
    return c;
}
const char* layout_tool_name(unsigned id){return id<LAYOUT_TOOL_COUNT?tools[id]:nullptr;}
static bool parse(char* text,size_t length,const LayoutConfig& base,LayoutConfig& output,bool complete){
    if(!text||length>LAYOUT_MAX_BYTES||memchr(text,0,length))return false;
    // Caller supplies capacity length+1.
    text[length]=0;
    if(length>=3&&uint8_t(text[0])==0xef&&uint8_t(text[1])==0xbb&&uint8_t(text[2])==0xbf)text+=3;
    LayoutConfig c=base;uint16_t seen=0;
    for(char* line=text;line;){
        char* next=strchr(line,'\n');if(next)*next++=0;
        char* hash=strchr(line,'#');if(hash)*hash=0;
        line=trim(line);
        if(*line){
            char* eq=strchr(line,'=');if(!eq)return false;*eq++=0;
            int key=find(trim(line),keys);char* value=trim(eq);
            if(key<0||(seen&(1u<<key)))return false;
            seen|=1u<<key;
            if(key<6){
                int id=find(value,targets);if(id<0)return false;
                c.home[key]=static_cast<HomeTarget>(id);
            }else if(key==6){
                uint8_t order[LAYOUT_TOOL_COUNT],count;
                if(!list(value,tools,order,count))return false;
                bool used[LAYOUT_TOOL_COUNT]={};
                for(unsigned i=0;i<count;++i){c.tools[i]=order[i];used[order[i]]=true;}
                for(unsigned i=0;i<base.tool_count;++i)if(!used[base.tools[i]])c.tools[count++]=base.tools[i];
            }else if(key==7){
                uint8_t ids[LAYOUT_TOOL_COUNT],count;
                if(!list(value,tools,ids,count))return false;
                memset(c.hidden,0,sizeof(c.hidden));
                for(unsigned i=0;i<count;++i)c.hidden[ids[i]]=true;
            }else if(key==8){
                if(!list(value,controls,c.controls,c.control_count))return false;
            }else if(key==9){
                if(!list(value,actions,c.actions,c.action_count))return false;
            }
            // The two inventory keys are informational only. They cannot
            // register screens or alter the compiled shortcut allowlist.
        }
        line=next;
    }
    if(complete && (seen&((1u<<10)-1))!=((1u<<10)-1))return false;
    // Never strand the user without access to Settings.
    bool quick_settings=false, reachable=false;
    for(unsigned i=0;i<c.action_count;++i)if(c.actions[i]==0)quick_settings=true;
    for(auto t:c.home)if(t==HomeTarget::Settings||(t==HomeTarget::QuickControls&&quick_settings))reachable=true;
    if(!reachable)return false;
    output=c;return true;
}


// Parse once, including before hardware setup. The only emergency mapping below
// is a minimal Settings/Tools escape hatch if a malformed canonical file was
// compiled; CI explicitly rejects that condition.
namespace {
struct EmbeddedLayout {
    LayoutConfig config=inventory();
    bool valid=false;
    EmbeddedLayout(){
        // Avoid placing the entire reference inventory on the early boot stack.
        auto* text=static_cast<char*>(malloc(sizeof(layout_embedded_text)));
        if(text){
            memcpy(text,layout_embedded_text,sizeof(layout_embedded_text));
            valid=parse(text,sizeof(layout_embedded_text)-1,config,config,true);
            free(text);
        }
        if(!valid){
            config.home[static_cast<unsigned>(HomeInput::Back)]=HomeTarget::Settings;
            config.home[static_cast<unsigned>(HomeInput::SwipeUp)]=HomeTarget::Tools;
        }
    }
};
const EmbeddedLayout& embedded(){
    static const EmbeddedLayout result;
    return result;
}
}
bool layout_embedded_valid(){return embedded().valid;}
LayoutConfig layout_defaults(){return embedded().config;}
bool layout_parse_overlay(char* text,size_t length,const LayoutConfig& base,LayoutConfig& output){
    return parse(text,length,base,output,false);
}
bool layout_parse(char* text,size_t length,LayoutConfig& output){
    return layout_parse_overlay(text,length,layout_defaults(),output);
}
