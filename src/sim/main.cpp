#include <lvgl.h>
#include <array>
#include <cstdint>
#include <cstdio>
namespace {
constexpr int width = 502, height = 410;
std::array<uint16_t, width * 32> buffer{};
unsigned flushes = 0;
void flush(lv_display_t* display, const lv_area_t*, uint8_t*) {
    ++flushes;
    lv_display_flush_ready(display);
}
}
int main() {
    lv_init();
    auto* display = lv_display_create(width, height);
    if (!display) return 1;
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);
    lv_display_set_buffers(display, buffer.data(), nullptr, sizeof(buffer), LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, flush);
    auto* label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "T-Watch Ultra native runtime");
    lv_obj_center(label);
    for (int i = 0; i < 20; ++i) {
        lv_tick_inc(10);
        lv_timer_handler();
    }
    const bool ok = flushes > 0;
    std::printf("LVGL headless smoke: %u flushes\n", flushes);
    lv_display_delete(display);
    lv_deinit();
    return ok ? 0 : 1;
}
