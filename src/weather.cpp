#include "weather.h"
#include "radio_manager.h"
#include "event_manager.h"
#include "wifi_creds.h"
#include "gps_screen.h"
#include <Arduino.h>
#include <LilyGoLib.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Preferences.h>
#include <new>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ---- module state ---------------------------------------------------------

static WeatherSample s_sample   = {};
static uint32_t      s_saved_at_millis = 0;  // millis() when we last set s_sample
static uint32_t      s_saved_at_epoch  = 0;  // wall-clock secs when we saved (for cross-boot age)
static const char   *s_status   = "";
static bool s_fetching = false; // UI-owned
static char s_http_status[24];

// NVS: single blob, so we serialise the whole sample plus timestamps.
static const char *NVS_NS        = "weather";
static const char *NVS_KEY       = "sample";

// Package written to NVS. Explicit sizes (int32_t / float32) so a future
// firmware can still parse an older blob without alignment surprises.
struct __attribute__((packed)) WeatherBlob {
    uint32_t magic;         // 0x5757 = "WW"
    uint32_t saved_epoch;   // wall-clock secs when saved (0 if RTC was unknown)
    float    temp_c;
    int16_t  humidity_pct;
    int16_t  pressure_hpa;
    uint8_t  valid;
    uint8_t  reserved[3];
};

// ---- persistence -----------------------------------------------------------

static void nvs_save(const WeatherSample& sample, uint32_t epoch)
{
    Preferences p;
    if (!p.begin(NVS_NS, false)) return;
    WeatherBlob b = {};
    b.magic        = 0x00005757;
    b.saved_epoch  = epoch;
    b.temp_c       = sample.temp_c;
    b.humidity_pct = (int16_t)sample.humidity_pct;
    b.pressure_hpa = (int16_t)sample.pressure_hpa;
    b.valid        = sample.valid ? 1 : 0;
    p.putBytes(NVS_KEY, &b, sizeof(b));
    p.end();
}

void weather_init()
{
    Preferences p;
    if (!p.begin(NVS_NS, true)) return;
    WeatherBlob b = {};
    size_t n = p.getBytes(NVS_KEY, &b, sizeof(b));
    p.end();
    if (n != sizeof(b) || b.magic != 0x00005757) return;
    s_sample.valid        = (b.valid != 0);
    s_sample.temp_c       = b.temp_c;
    s_sample.humidity_pct = b.humidity_pct;
    s_sample.pressure_hpa = b.pressure_hpa;
    s_sample.age_ms       = 0;   // recomputed on read
    s_saved_at_millis     = millis();
    s_saved_at_epoch      = b.saved_epoch;
    if (s_sample.valid) s_status = "loaded";
}

// ---- JSON parse (tiny, targeted) ------------------------------------------
//
// Open-Meteo's response for our query is small and predictable:
//   {"latitude":..., "current":{"temperature_2m":22.4,
//    "relative_humidity_2m":65, "surface_pressure":1013.2}, ...}
// We only care about three numbers under "current". Hand-parsing avoids
// pulling in ArduinoJson for 3 fields.

static bool find_number(const String &body, const char *key, float *out)
{
    int k = body.indexOf(key);
    if (k < 0) return false;
    // Skip to the ':' after the key, then read the number.
    int colon = body.indexOf(':', k);
    if (colon < 0) return false;
    int i = colon + 1;
    while (i < (int)body.length() && (body[i] == ' ' || body[i] == '\t')) i++;
    int start = i;
    while (i < (int)body.length() &&
           (isdigit(body[i]) || body[i] == '.' || body[i] == '-'))
        i++;
    if (i == start) return false;
    *out = body.substring(start, i).toFloat();
    return true;
}

// ---- fetch task -----------------------------------------------------------
//
// Runs on core 0 (WiFi core), so we don't have to hop cores on every http
// call. A single request borrows an existing station or owns a temporary
// connection, then releases its lease before exiting.

struct FetchArgs {
    float lat, lon;
    char ssid[33], pass[65];
};
enum WeatherStatus { WifiUp, Fetching, Timeout, Busy, HttpError, ParseError, Success };
static void post(WeatherStatus status, bool done, int http_code=0)
{
    UiEvent e;e.kind=UiEventKind::Weather;e.weather.status=status;
    e.weather.done=done;e.value=http_code;event_publish(e);
}
static void fetch_task(void *arg)
{
    // All request inputs are immutable copies from the UI task.
    FetchArgs a = *static_cast<FetchArgs*>(arg);
    delete static_cast<FetchArgs*>(arg);
    if(!radio_job_begin(RadioJob::Weather)){
        post(Busy,true);vTaskDelete(nullptr);return;
    }
    const bool owned=WiFi.getMode()==WIFI_OFF;
    if(!owned && WiFi.status()!=WL_CONNECTED){
        radio_job_end(RadioJob::Weather);post(Busy,true);vTaskDelete(nullptr);return;
    }
    post(WifiUp,false);
    if(owned && a.ssid[0] && WiFi.mode(WIFI_STA)) WiFi.begin(a.ssid,a.pass);
    uint32_t t0 = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - t0 < 6000 && !radio_manager_stats().suspended)
        vTaskDelay(pdMS_TO_TICKS(120));
    if (WiFi.status() != WL_CONNECTED || radio_manager_stats().suspended) {
        if(owned){WiFi.disconnect(true,false);WiFi.mode(WIFI_OFF);}
        radio_job_end(RadioJob::Weather);
        post(Timeout,true);
        vTaskDelete(NULL);return;
    }

    post(Fetching,false);
    UiEvent e;e.kind=UiEventKind::Weather;e.weather.done=true;
    {
        char url[192];
        snprintf(url,sizeof(url),
                 "https://api.open-meteo.com/v1/forecast"
                 "?latitude=%.4f&longitude=%.4f"
                 "&current=temperature_2m,relative_humidity_2m,surface_pressure",
                 a.lat,a.lon);
        HTTPClient http;
        http.setTimeout(8000);http.setConnectTimeout(4000);
        http.begin(url);
        int code=http.GET();
        if(code!=200){
            e.weather.status=HttpError;e.value=code;
        }else{
            String body=http.getString();
            int cur=body.indexOf("\"current\"");
            String cbody=body.substring(cur<0?0:cur);
            float t=0,h=0,p=0;
            if(find_number(cbody,"temperature_2m",&t) &&
               find_number(cbody,"relative_humidity_2m",&h) &&
               find_number(cbody,"surface_pressure",&p)){
                WeatherSample sample{true,t,int(h+0.5f),int(p+0.5f),0};
                time_t now=time(nullptr);
                e.weather.status=Success;e.weather.temp_c=sample.temp_c;
                e.weather.humidity=sample.humidity_pct;e.weather.pressure=sample.pressure_hpa;
                e.weather.saved_at=millis();
                e.weather.saved_epoch=now>1600000000L?uint32_t(now):0;
                nvs_save(sample,e.weather.saved_epoch);
            }else e.weather.status=ParseError;
        }
        http.end();
    } // release HTTP objects before deleting this task
    if(owned){WiFi.disconnect(true,false);WiFi.mode(WIFI_OFF);}
    radio_job_end(RadioJob::Weather);
    // Final result comes only after radio teardown and persistence complete.
    event_publish(e);
    vTaskDelete(NULL);
}
void weather_bg_tick(){
    UiEvent e;
    if(!ui_task_is_owner() || !event_take(UiEventKind::Weather,e))return;
    switch(e.weather.status){
        case WifiUp:s_status="wifi up...";break;
        case Fetching:s_status="fetching...";break;
        case Timeout:s_status="wifi timeout";break;
        case Busy:s_status="radio busy";break;
        case HttpError:
            snprintf(s_http_status,sizeof(s_http_status),"http %d",e.value);
            s_status=s_http_status;break;
        case ParseError:s_status="json parse";break;
        case Success:
            s_sample={true,e.weather.temp_c,e.weather.humidity,e.weather.pressure,0};
            s_saved_at_millis=e.weather.saved_at;s_saved_at_epoch=e.weather.saved_epoch;
            s_status="ok";break;
    }
    if(e.weather.done)s_fetching=false;
}

// ---- public API -----------------------------------------------------------

bool weather_is_fetching() { return s_fetching; }

bool weather_fetch_async()
{
    if (s_fetching) return false;
    if(radio_job_busy()){s_status="radio busy";return false;}
    // Need a GPS fix so we can send lat/lon. The clock face taps the
    // widget; if the user has no fix yet, that's a clean "—" state.
    if (!gps_screen_has_lock() ||
        !instance.gps.location.isValid()) {
        s_status = "no gps";
        return false;
    }

    // Credential fallback may read SD; keep that on its existing owner.
    FetchArgs *a = new(std::nothrow) FetchArgs{};
    if(!a){s_status="out of memory";return false;}
    if(WiFi.status()!=WL_CONNECTED && !wifi_creds_get(a->ssid,sizeof(a->ssid),a->pass,sizeof(a->pass))){
        delete a;s_status="no wifi creds";return false;
    }
    a->lat = (float)instance.gps.location.lat();
    a->lon = (float)instance.gps.location.lng();

    s_fetching = true;
    s_status = "starting...";
    BaseType_t ok = xTaskCreatePinnedToCore(fetch_task, "weather",
                                            8192, a, 2, nullptr, 0);
    if (ok != pdPASS) {
        s_fetching = false;
        s_status = "task spawn failed";
        delete a;
        return false;
    }
    return true;
}

WeatherSample weather_get()
{
    WeatherSample r = s_sample;
    r.age_ms = s_saved_at_millis ? (millis() - s_saved_at_millis) : 0;
    return r;
}

const char *weather_last_status_text()
{
    return s_status ? s_status : "";
}
