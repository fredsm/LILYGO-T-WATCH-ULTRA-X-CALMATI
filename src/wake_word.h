#pragma once
#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------------------
// Wake-word engine — offline "Hi ESP" detection built on Espressif's ESP-SR
// (WakeNet9), vendored into lib/esp-sr/ as precompiled static libraries.
//
// Uses the shared microphone lease and drives WakeNet directly on core 0.
// Model loading, capture and detection never execute on the LVGL task.
// The UI consumes the detection flag; no worker calls LVGL.
//
// See docs/esp-sr-plan.md for the full research + decision log that led to
// this vendored setup.
// ---------------------------------------------------------------------------

// Request asynchronous model loading and detection. True means admitted;
// status reports model/driver failures. Recording and detection share one mic.
bool wake_word_start();
// Nonblocking stop request. Ownership releases only after model/I2S cleanup.
void wake_word_stop();
// True during startup, detection and cleanup.
bool wake_word_is_active();

// Last audio energy the AFE handed us, mapped to 0..100 for the KITT bars.
// Meant for UI polish, not analysis; returns 0 when the engine isn't running.
int  wake_word_get_energy();

// Human-readable last status. Returns "" until wake_word_start is first
// called, then one of: "loading models", "listening", "wake!", "AFE alloc
// failed", "model not in partition", "partition 'model' missing".
const char *wake_word_status_text();

// True once since the last wake_word_consume_detection(). Cleared by that
// call. The UI checks this from its tick, so no callback plumbing needed
// for the first cut.
bool wake_word_pending_detection();
void wake_word_consume_detection();
