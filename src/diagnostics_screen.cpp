#include "diagnostics_screen.h"
#include "diagnostics_manager.h"
#include "event_manager.h"
#include "gps_service.h"
#include "storage_manager.h"
#include "spi_bus_manager.h"
#include "i2c_bus_manager.h"
#include "qspi_display.h"
#include "task_manager.h"
#include <lvgl.h>
#include <stdio.h>

namespace {
lv_obj_t *screen=nullptr, *status=nullptr, *cpu=nullptr, *ui=nullptr, *memory=nullptr,
         *timings=nullptr, *history=nullptr, *spi=nullptr, *i2c=nullptr, *qspi=nullptr;
lv_timer_t* timer=nullptr;
lv_obj_t* label(lv_obj_t* parent, const char* text, bool heading=false) {
    auto* obj=lv_label_create(parent);
    lv_obj_set_width(obj,lv_pct(100));
    lv_label_set_long_mode(obj,LV_LABEL_LONG_WRAP);
    lv_obj_set_style_text_font(obj,heading?&lv_font_montserrat_20:&lv_font_montserrat_16,0);
    lv_obj_set_style_text_color(obj,heading?lv_color_hex(0x62D5F4):lv_color_hex(0xE0E0E0),0);
    lv_label_set_text(obj,text);
    return obj;
}
void refresh() {
    const auto d=diagnostics_snapshot();
    const auto events=event_stats();
    const auto sd=storage_stats();
    lv_label_set_text_fmt(status,"%s%s | Uptime %lu s\nReset: %s",
        d.simulated?"SIMULATED | ":"",!d.ready?"Monitor unavailable":d.paused?"Paused":d.stalled?"STALLED":"Monitoring",
        (unsigned long)(d.uptime_ms/1000),d.reset_reason);
    char a[12],b[12];
    if (d.core0<0) snprintf(a,sizeof(a),"N/A"); else snprintf(a,sizeof(a),"%d%%",d.core0);
    if (d.core1<0) snprintf(b,sizeof(b),"N/A"); else snprintf(b,sizeof(b),"%d%%",d.core1);
    lv_label_set_text_fmt(cpu,"Frequency: %lu MHz\nCore 0: %s   Core 1: %s",
        (unsigned long)d.cpu_mhz,a,b);
    lv_label_set_text_fmt(ui,"UI handler: %lu ms\nMax UI gap: %lu ms   Stalls: %lu\nActive: %s\nEvents pending: %u | Coalesced: %lu\nBack edges merged: %lu\nGPS UART backlog peak: %lu B\nJob slice max: %lu ms | Over 4 ms: %lu\nSD queue: %u | %lu KB%s\nSD done: %lu | Failed: %lu | Rejected: %lu | Cancelled: %lu\nSD merged: %lu | Max job: %lu ms",
        (unsigned long)d.last_ui_ms,(unsigned long)d.max_ui_gap_ms,(unsigned long)d.stalls,
        diagnostics_stage_name(d.active),events.pending,(unsigned long)events.replaced,
        (unsigned long)events.back_coalesced,(unsigned long)gps_service_backlog(),
        (unsigned long)d.max_slice_ms,(unsigned long)d.slice_overruns,sd.pending,
        (unsigned long)(sd.queued_bytes/1024),sd.active?" (working)":"",
        (unsigned long)sd.completed,(unsigned long)sd.failed,(unsigned long)sd.rejected,(unsigned long)sd.cancelled,(unsigned long)sd.coalesced,(unsigned long)sd.max_job_ms);
    const auto bus=spi_bus_stats();
    char bus_text[480];
    int bytes=snprintf(bus_text,sizeof(bus_text),"Owner: %s | Waiting: %lu\nActive: %lu us",
        bus.active?spi_bus_client_name(bus.owner):"Idle",(unsigned long)bus.waiters,(unsigned long)bus.active_us);
    size_t pos=bytes>0?size_t(bytes):0;
    for(unsigned i=0;i<static_cast<unsigned>(SpiClient::Count) && pos<sizeof(bus_text);++i) {
        const auto& c=bus.clients[i];
        const int n=snprintf(bus_text+pos,sizeof(bus_text)-pos,
            "\n%s: %lu tx | %lu contended | %lu deferred\nMax wait: %lu us | Hold: %lu us",
            spi_bus_client_name(static_cast<SpiClient>(i)),(unsigned long)c.transactions,
            (unsigned long)c.contended,(unsigned long)c.deferred,(unsigned long)c.max_wait_us,(unsigned long)c.max_hold_us);
        if(n<0 || size_t(n)>=sizeof(bus_text)-pos)break;
        pos+=size_t(n);
    }
    lv_label_set_text(spi,bus_text);
    const auto wire=i2c_bus_stats();
    static char wire_text[1024];
    bytes=snprintf(wire_text,sizeof(wire_text),
        "%s | Failures: %lu\nRecovery: %s | Resumed: %lu\nAdmission timeouts: %lu | Active: %lu us",
        wire.cooling?"Cooldown":wire.active?i2c_client_name(i2c_client(wire.address)):"Idle",
        (unsigned long)wire.failures,wire.recovering?"Pending":"OK",(unsigned long)wire.recoveries,
        (unsigned long)wire.admission_timeouts,(unsigned long)wire.active_us);
    pos=bytes>0?size_t(bytes):0;
    for(unsigned i=0;i<static_cast<unsigned>(I2cClient::Count) && pos<sizeof(wire_text);++i){
        const auto& c=wire.clients[i];
        const int n=snprintf(wire_text+pos,sizeof(wire_text)-pos,
            "\n%s: %lu tx | %lu NACK | %lu timeout\n%lu other err | %lu deferred\nMax wait: %lu us | Transfer: %lu us",
            i2c_client_name(static_cast<I2cClient>(i)),(unsigned long)c.transactions,
            (unsigned long)c.nacks,(unsigned long)c.timeouts,(unsigned long)c.errors,(unsigned long)c.deferred,
            (unsigned long)c.max_wait_us,(unsigned long)c.max_transfer_us);
        if(n<0 || size_t(n)>=sizeof(wire_text)-pos)break;
        pos+=size_t(n);
    }
    lv_label_set_text(i2c,wire_text);
    const auto display=qspi_display_stats();
    lv_label_set_text_fmt(qspi,
        "%s | %s\nFlushes: %lu | Completed: %lu\nDropped: %lu | Commands: %lu\nTimeouts: %lu | Errors: %lu | Recovered: %lu\nActive: %lu us | Max wait: %lu us\nMax submit: %lu us | Queued-to-reap: %lu us\nPixel data: %lu KB | DMA staging: %u KB",
        display.enabled?"Async DMA":"Synchronous / simulated",
        display.quarantined?"QUARANTINED":display.pending?"In flight":display.paused?"Paused":display.cooling?"Cooldown":"Idle",
        (unsigned long)display.flushes,(unsigned long)display.completed,(unsigned long)display.dropped,(unsigned long)display.commands,
        (unsigned long)display.timeouts,(unsigned long)display.errors,(unsigned long)display.recoveries,
        (unsigned long)display.active_us,(unsigned long)display.max_wait_us,(unsigned long)display.max_submit_us,
        (unsigned long)display.max_transfer_us,(unsigned long)(display.bytes/1024),unsigned(QspiPipeline::capacity/1024));
    if (!d.ready || !d.memory_sample_ms) lv_label_set_text(memory,"Memory sample pending");
    else lv_label_set_text_fmt(memory,"Internal free: %lu KB\nMinimum free: %lu KB\nLargest block: %lu KB\nFree PSRAM: %lu KB\nUI stack headroom: %lu B\nMemory sample age: %lu s",
        (unsigned long)(d.heap/1024),(unsigned long)(d.min_heap/1024),
        (unsigned long)(d.largest/1024),(unsigned long)(d.psram/1024),(unsigned long)d.ui_stack,
        (unsigned long)(d.uptime_ms>=d.memory_sample_ms?(d.uptime_ms-d.memory_sample_ms)/1000:0));
    // A compact ranking, excluding idle and not-yet-run operations.
    char text[640]=""; size_t used=0;
    bool picked[static_cast<unsigned>(DiagStage::Count)]={};
    for (unsigned j=0;j<6;++j) {
        unsigned best=0;
        for (unsigned i=1;i<static_cast<unsigned>(DiagStage::Count);++i)
            if (!picked[i] && d.timings[i].count && (!best || d.timings[i].maximum>d.timings[best].maximum)) best=i;
        if (!best) break;
        picked[best]=true;
        int n=snprintf(text+used,sizeof(text)-used,"%s%s: %lu ms max",used?"\n":"",
            diagnostics_stage_name(static_cast<DiagStage>(best)),(unsigned long)d.timings[best].maximum);
        if(n<0 || size_t(n)>=sizeof(text)-used)break;
        used+=size_t(n);
    }
    lv_label_set_text(timings,used?text:"No samples yet");
    used=0; text[0]=0;
    for(unsigned i=0;i<d.history_count && i<5;++i){
        const auto& r=d.history[i];
        int n=snprintf(text+used,sizeof(text)-used,"%s%lu ms %s at %lu s\n%s",used?"\n\n":"",
            (unsigned long)r.duration,r.recovered?"recovered":"ongoing",
            (unsigned long)(r.at/1000),diagnostics_stage_name(r.stage));
        if(n<0 || size_t(n)>=sizeof(text)-used)break;
        used+=size_t(n);
    }
    lv_label_set_text(history,used?text:"No stalls >= 100 ms");
}
void back(lv_event_t*) { task_manager_show(); }
void create() {
    screen=lv_obj_create(nullptr);
    lv_obj_set_style_bg_color(screen,lv_color_black(),0);
    lv_obj_clear_flag(screen,LV_OBJ_FLAG_SCROLLABLE);
    auto* button=lv_button_create(screen); lv_obj_set_size(button,58,38); lv_obj_set_pos(button,10,10);
    auto* arrow=lv_label_create(button); lv_label_set_text(arrow,LV_SYMBOL_LEFT); lv_obj_center(arrow);
    lv_obj_add_event_cb(button,back,LV_EVENT_CLICKED,nullptr);
    auto* title=lv_label_create(screen);lv_label_set_text(title,"SYSTEM DIAGNOSTICS");
    lv_obj_set_style_text_font(title,&lv_font_montserrat_20,0);lv_obj_set_style_text_color(title,lv_color_white(),0);
    lv_obj_align(title,LV_ALIGN_TOP_MID,25,20);
    auto* body=lv_obj_create(screen);lv_obj_set_size(body,lv_pct(100),lv_pct(100));
    lv_obj_set_height(body,445);lv_obj_align(body,LV_ALIGN_BOTTOM_MID,0,0);
    lv_obj_set_style_border_width(body,0,0);lv_obj_set_style_bg_opa(body,LV_OPA_TRANSP,0);
    lv_obj_set_style_pad_all(body,16,0);lv_obj_set_style_pad_row(body,10,0);
    lv_obj_set_flex_flow(body,LV_FLEX_FLOW_COLUMN);
    status=label(body,"");
    label(body,"CPU",true);cpu=label(body,"");
    label(body,"RESPONSIVENESS",true);ui=label(body,"");
    label(body,"SHARED SPI BUS",true);spi=label(body,"");
    label(body,"SHARED I2C BUS",true);i2c=label(body,"");
    label(body,"DISPLAY QSPI DMA",true);qspi=label(body,"");
    label(body,"MEMORY",true);memory=label(body,"");
    label(body,"SLOWEST OPERATIONS",true);timings=label(body,"");
    label(body,"RECENT UI STALLS",true);history=label(body,"");
    label(body,"Operation timings include driver calls and waiting, not raw bus transactions. SPI timings cover bus transactions only. History is held in RAM.");
    timer=lv_timer_create([](lv_timer_t*){if(diagnostics_screen_is_active())refresh();},1000,nullptr);
}
}
void diagnostics_screen_show(){if(!screen)create();refresh();lv_screen_load(screen);}
bool diagnostics_screen_is_active(){return screen && lv_screen_active()==screen;}
