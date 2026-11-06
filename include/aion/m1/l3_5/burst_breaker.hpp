#pragma once
#include <cstdint>
namespace aion::m1::l3_5 {
struct BurstBreaker {
    uint64_t limit_per_tick{100};
    uint64_t count{0};
    uint64_t last_tick{0};
    uint64_t total_throttled{0}; // 累计节流次数

    bool admit(uint64_t current_tick) noexcept {
        if (current_tick != last_tick) { last_tick = current_tick; count = 0; }
        if (count >= limit_per_tick) {
            ++total_throttled;
            return false;
        }
        ++count; return true;
    }
};
} // namespace aion::m1::l3_5
