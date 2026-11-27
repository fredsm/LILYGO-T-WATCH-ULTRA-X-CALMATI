#include "timezone.h"
#include "radio_manager.h"
#include "event_manager.h"
#include "status_cache.h"
#include <Arduino.h>
#include <LilyGoLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include "storage_text.h"
#include <time.h>
#include <limits.h>
#include "usb_sd.h"

// Implemented in main.cpp (clock screen). The RTC holds UTC; clock_utc_offset
// shifts it to local for display.
void clock_screen_set_utc_offset(int offset_hours);
bool clock_screen_manual_time_active();
void clock_screen_refresh();

#define TZ_PATH "/Settings/timezone.txt"

// ---- persistence -----------------------------------------------------------

static int s_last_written = INT_MIN;   // avoid redundant SD writes

void timezone_note_detected(int offset_hours)
{
    if (offset_hours == s_last_written) return;            // unchanged
    if (offset_hours < -12 || offset_hours > 14) return;   // sanity
    if (!storage_stats().accepting) return;

    StorageText f;
    f.printf("offset=%d\n", offset_hours);
    if (!f.commit(TZ_PATH, StorageMode::Replace)) return;
    s_last_written = offset_hours;
}

void timezone_load_on_boot()
{
    StorageAccess sd_access;
    if (!sd_access) { return; }
    // Manual Time wins — in that mode the RTC already holds local time directly.
    if (clock_screen_manual_time_active()) return;
    if (!instance.isCardReady() || usb_sd_is_running()) return;
    if (!SD.exists(TZ_PATH)) return;

    File f = SD.open(TZ_PATH, FILE_READ);
    if (!f) return;
    char buf[32] = {0};
    int n = f.readBytesUntil('\n', (uint8_t *)buf, sizeof(buf) - 1);
    f.close();
    if (n <= 0) return;
    buf[n] = '\0';

    const char *p = strstr(buf, "offset=");
    if (!p) return;
    int off = atoi(p + 7);
    if (off < -12 || off > 14) return;     // sanity

    s_last_written = off;                  // already on disk; don't rewrite it
    clock_screen_set_utc_offset(off);
    clock_screen_refresh();
}

// ---- WiFi NTP + IP-geolocation background sync ------------------------------
//
// NTP and HTTP both block for seconds, so the network work runs on a dedicated
// task; the results are applied back on the main loop (timezone_bg_tick) so the
// RTC / clock / SD are only touched from one context.

static TaskHandle_t s_task = nullptr;

// Query ip-api.com for the current UTC offset (seconds, DST-aware). Free tier
// is plain HTTP. Returns true and fills *out_h on success.
static bool http_get_offset(int *out_h)
{
    HTTPClient http;
    http.setConnectTimeout(5000);
    http.setTimeout(5000);
    if (!http.begin("http://ip-api.com/json/?fields=status,offset")) return false;

    bool ok = false;
    if (http.GET() == 200) {
        String body = http.getString();
        if (body.indexOf("\"status\":\"success\"") >= 0) {
            int i = body.indexOf("\"offset\":");
            if (i >= 0) {
                long secs = atol(body.c_str() + i + 9);
                *out_h = (int)(secs / 3600);
                ok = true;
            }
        }
    }
    http.end();
    return ok;
}

static void tz_worker(void *)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        if (WiFi.status() != WL_CONNECTED || !radio_job_begin(RadioJob::TimeSync)) continue;
        if(WiFi.status()!=WL_CONNECTED){radio_job_end(RadioJob::TimeSync);continue;}

        bool      got_time = false, got_off = false;
        int       off_h = 0;
        struct tm tmutc = {};

        // NTP: configTime(0,0,...) keeps system time in UTC.
        configTime(0, 0, "pool.ntp.org", "time.nist.gov");
        if (getLocalTime(&tmutc, 8000)) got_time = true;

        // IP geolocation -> current UTC offset.
        if (!radio_manager_stats().suspended && http_get_offset(&off_h)) got_off = true;
        radio_job_end(RadioJob::TimeSync);

        UiEvent result;
        result.kind=UiEventKind::Timezone;
        result.utc=tmutc;result.value=off_h;
        result.first=got_time;result.second=got_off;
        event_publish(result);
    }
}

static void on_wifi_got_ip(arduino_event_id_t, arduino_event_info_t)
{
    if(s_task)xTaskNotifyGive(s_task);
}

void timezone_init()
{
    if(!s_task && xTaskCreatePinnedToCore(tz_worker,"tz_sync",8192,nullptr,2,&s_task,0)==pdPASS)
        WiFi.onEvent(on_wifi_got_ip, ARDUINO_EVENT_WIFI_STA_GOT_IP);
}

void timezone_bg_tick()
{
    UiEvent result;
    if(!ui_task_is_owner() || !event_take(UiEventKind::Timezone,result))return;
    // Manual Time wins; apply RTC, SD and UI results in the existing owner.
    if(clock_screen_manual_time_active())return;
    if(result.first){
        instance.rtc.setDateTime(result.utc.tm_year+1900,result.utc.tm_mon+1,
            result.utc.tm_mday,result.utc.tm_hour,result.utc.tm_min,result.utc.tm_sec);
        instance.rtc.hwClockRead();
        status_cache_sample_clock();
    }
    if(result.second){
        clock_screen_set_utc_offset(result.value);
        timezone_note_detected(result.value);
    }
    clock_screen_refresh();
}
