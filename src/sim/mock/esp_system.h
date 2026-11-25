#pragma once
#include <cstdint>
// Deterministic fixture generator; no hardware RNG dependency.
inline uint32_t esp_random(){static uint32_t state=12345;state=state*1664525u+1013904223u;return state;}
