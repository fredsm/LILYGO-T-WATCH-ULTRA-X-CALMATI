#pragma once
#include <lvgl.h>

void nfc_screen_create();
void nfc_screen_show();
bool nfc_screen_is_active();
void nfc_screen_worker(); // call from loop() — no-op when idle

// Shared by the status screen and quick controls.
void nfc_screen_set_powered(bool on);
