#include "pin_lock_screen.h"
#include "power_manager.h"
#include <Preferences.h>
#include <cstring>

void clock_screen_show();
void main_loop_request_lvgl_priority(int cycles);

namespace {
constexpr char PIN[] = "1234";
constexpr uint32_t GREEN = 0x00ff80;
lv_obj_t *home = nullptr, *indicator = nullptr, *shield = nullptr;
lv_obj_t *unlock_screen = nullptr, *dots = nullptr, *message = nullptr;
bool active = false;
char entered[sizeof(PIN)] = {};
unsigned length = 0;

void icon_color(lv_obj_t* icon, lv_color_t color) {
    lv_obj_set_style_border_color(lv_obj_get_child(icon, 0), color, 0);
    lv_obj_set_style_bg_color(lv_obj_get_child(icon, 1), color, 0);
}

void persist(bool value) {
    Preferences prefs;
    if (prefs.begin("watchlock", false)) {
        prefs.putBool("locked", value);
        prefs.end();
    }
}

void update_home() {
    if (!home) return;
    icon_color(indicator, lv_color_hex(active ? GREEN : 0x333333));
    if (active) {
        lv_obj_clear_flag(shield, LV_OBJ_FLAG_HIDDEN);
        lv_obj_move_foreground(shield);
    } else {
        lv_obj_add_flag(shield, LV_OBJ_FLAG_HIDDEN);
    }
}

void update_dots() {
    char masked[] = "  O  O  O  O  ";
    // Never display the digit itself, even briefly.
    for (unsigned i = 0; i < length; ++i) masked[2 + i * 3] = '*';
    lv_label_set_text(dots, masked);
}

void clear_entry() {
    std::memset(entered, 0, sizeof(entered));
    length = 0;
    if (dots) update_dots();
    if (message) {
        lv_label_set_text(message, "Enter 4-digit PIN");
        lv_obj_set_style_text_color(message, lv_color_hex(0x9baab8), 0);
    }
}

void digit_pressed(lv_event_t* event) {
    if (!active || lv_screen_active() != unlock_screen) return;
    auto* button = static_cast<lv_obj_t*>(lv_event_get_target(event));
    const char* text = lv_label_get_text(lv_obj_get_child(button, 0));
    if (text[0] == 'C') { clear_entry(); return; }
    if (std::strcmp(text, LV_SYMBOL_BACKSPACE) == 0) {
        if (length) entered[--length] = '\0';
        update_dots();
        return;
    }
    if (length >= sizeof(PIN) - 1 || text[0] < '0' || text[0] > '9') return;
    if (!length) clear_entry();
    entered[length++] = text[0];
    update_dots();
    if (length != sizeof(PIN) - 1) return;

    if (std::strcmp(entered, PIN) == 0) {
        persist(false);
        active = false;
        clear_entry();
        update_home();
        // The successful fourth click is consumed by this screen.
        clock_screen_show();
    } else {
        clear_entry();
        lv_label_set_text(message, "Incorrect PIN - try again");
        lv_obj_set_style_text_color(message, lv_color_hex(0xff6666), 0);
    }
}

void build_keypad() {
    if (unlock_screen) return;
    auto* previous_display = lv_display_get_default();
    lv_display_set_default(lv_obj_get_display(home));
    unlock_screen = lv_obj_create(nullptr);
    lv_display_set_default(previous_display);
    lv_obj_clear_flag(unlock_screen, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_bg_color(unlock_screen, lv_color_hex(0x080e17), 0);
    lv_obj_set_style_pad_all(unlock_screen, 0, 0);
    lv_obj_set_style_border_width(unlock_screen, 0, 0);
    lv_obj_set_style_text_color(unlock_screen, lv_color_white(), 0);
    auto* icon = pin_lock_icon_create(unlock_screen, lv_color_hex(GREEN), 30);
    lv_obj_align(icon, LV_ALIGN_TOP_MID, 0, 18);
    auto* title = lv_label_create(unlock_screen);
    lv_label_set_text(title, "UNLOCK WATCH");
    lv_obj_set_style_text_font(title, &lv_font_montserrat_20, 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 56);
    dots = lv_label_create(unlock_screen);
    lv_obj_set_style_text_font(dots, &lv_font_montserrat_28, 0);
    lv_obj_set_style_text_color(dots, lv_color_hex(GREEN), 0);
    lv_obj_align(dots, LV_ALIGN_TOP_MID, 0, 92);
    message = lv_label_create(unlock_screen);
    lv_obj_set_style_text_font(message, &lv_font_montserrat_16, 0);
    lv_obj_align(message, LV_ALIGN_TOP_MID, 0, 135);

    const char* keys[] = {"1","2","3","4","5","6","7","8","9","Clear","0",LV_SYMBOL_BACKSPACE};
    for (unsigned i = 0; i < 12; ++i) {
        auto* button = lv_button_create(unlock_screen);
        lv_obj_set_size(button, 110, 67);
        lv_obj_set_pos(button, 30 + (i % 3) * 120, 166 + (i / 3) * 77);
        lv_obj_set_style_radius(button, 14, 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x172331), 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0x125348), LV_STATE_PRESSED);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0x344657), 0);
        auto* label = lv_label_create(button);
        lv_label_set_text(label, keys[i]);
        lv_obj_set_style_text_font(label, i == 9 ? &lv_font_montserrat_16 : &lv_font_montserrat_28, 0);
        lv_obj_center(label);
        lv_obj_clear_flag(label, LV_OBJ_FLAG_CLICKABLE);
        lv_obj_add_event_cb(button, digit_pressed, LV_EVENT_CLICKED, nullptr);
    }
    clear_entry();
}

void show_keypad() {
    if (!active || power_screen_off() || power_shutting_down() || power_is_asleep()) return;
    if (lv_screen_active() == unlock_screen) return;
    build_keypad();
    clear_entry();
    main_loop_request_lvgl_priority(8);
    lv_screen_load(unlock_screen);
}
}

lv_obj_t* pin_lock_icon_create(lv_obj_t* parent, lv_color_t color, int size) {
    auto* icon = lv_obj_create(parent);
    lv_obj_remove_style_all(icon);
    lv_obj_set_size(icon, size, size);
    lv_obj_clear_flag(icon, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    auto* shackle = lv_obj_create(icon);
    lv_obj_remove_style_all(shackle);
    lv_obj_set_size(shackle, size * 3 / 5, size * 3 / 5);
    lv_obj_set_style_radius(shackle, size / 4, 0);
    lv_obj_set_style_border_width(shackle, size >= 26 ? 3 : 2, 0);
    lv_obj_set_style_border_color(shackle, color, 0);
    lv_obj_align(shackle, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_clear_flag(shackle, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    auto* body = lv_obj_create(icon);
    lv_obj_remove_style_all(body);
    lv_obj_set_size(body, size * 4 / 5, size * 3 / 5);
    lv_obj_set_style_bg_opa(body, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(body, color, 0);
    lv_obj_set_style_radius(body, size / 8, 0);
    lv_obj_align(body, LV_ALIGN_BOTTOM_MID, 0, 0);
    lv_obj_clear_flag(body, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_CLICKABLE));
    return icon;
}

void pin_lock_init(lv_obj_t* clock) {
    if (home) return;
    home = clock;
    indicator = pin_lock_icon_create(home, lv_color_hex(0x333333), 20);
    lv_obj_align(indicator, LV_ALIGN_TOP_LEFT, 18, 20);

    // A transparent, non-bubbling target covers the clock without changing its
    // appearance. Interact on press, then wait for release so it cannot type.
    shield = lv_obj_create(home);
    lv_obj_remove_style_all(shield);
    lv_obj_set_size(shield, lv_pct(100), lv_pct(100));
    lv_obj_center(shield);
    lv_obj_clear_flag(shield, static_cast<lv_obj_flag_t>(LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_GESTURE_BUBBLE | LV_OBJ_FLAG_EVENT_BUBBLE));
    lv_obj_add_flag(shield, LV_OBJ_FLAG_CLICKABLE);
    auto open = [](lv_event_t* e) {
        if (!active || lv_screen_active() != home) return;
        auto* input = lv_event_get_indev(e);
        if (input) lv_indev_wait_release(input);
        show_keypad();
    };
    lv_obj_add_event_cb(shield, open, LV_EVENT_PRESSED, nullptr);
    // Also catch presses on the clock's own padding, if any.
    lv_obj_add_event_cb(home, open, LV_EVENT_PRESSED, nullptr);

    pin_lock_restore();
}

void pin_lock_restore() {
    active = false;
    Preferences prefs;
    if (prefs.begin("watchlock", true)) {
        active = prefs.getBool("locked", false);
        prefs.end();
    }
    clear_entry();
    update_home();
}

void pin_lock_activate() {
    if (!home || active) return;
    active = true;
    persist(true);
    clear_entry();
    update_home();
    clock_screen_show();
}
bool pin_lock_active() { return active; }
bool pin_lock_unlock_screen_active() { return active && unlock_screen && lv_screen_active() == unlock_screen; }
bool pin_lock_handle_button() {
    if (!active) return false;
    show_keypad();
    return true;
}
void pin_lock_on_screenoff() {
    if (!active || !home) return;
    clear_entry();
    if (lv_screen_active() != home) lv_screen_load(home);
}
void pin_lock_enforce() {
    if (!active || !home) return;
    if (lv_screen_active() != home && lv_screen_active() != unlock_screen) {
        clear_entry();
        lv_screen_load(home);
    }
}
