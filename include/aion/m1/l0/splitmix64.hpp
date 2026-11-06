#pragma once
#include <cstdint>
namespace aion::m1::l0 {
struct SplitMix64 {
    uint64_t state{0};
    explicit SplitMix64(uint64_t seed) noexcept : state(seed) {}
    uint64_t next() noexcept {
        uint64_t z = (state += 0x9E3779B97F4A7C15ULL);
        z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
        z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
        return z ^ (z >> 31);
    }
    float next_float() noexcept {
        // [0,1)
        return float(next() >> 40) * (1.0f / 16777216.0f);
    }
};
} // namespace aion::m1::l0
