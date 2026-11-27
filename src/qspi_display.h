#pragma once
#include "qspi_display_pipeline.h"
// UI task only. Pixel DMA never references LVGL's render buffers.
void qspi_display_init();
void qspi_display_pause(bool paused);
bool qspi_display_idle();
bool qspi_display_drain();
void qspi_display_service();
bool qspi_display_prepare_sleep(); // drain and acknowledge panel SLPIN
bool qspi_display_panel_awake();
QspiStats qspi_display_stats();
