#pragma once
#include <cstdint>
struct SimSD {
 uint64_t totalBytes() const { return 32000000000ULL; }
 uint64_t usedBytes() const { return 4000000ULL; }
};
inline SimSD SD;
