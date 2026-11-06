#pragma once
#include <cstdint>
namespace aion::m1::l3_5 {
struct FreezeCounter {
    uint32_t count{0};
    bool should_force_thaw() noexcept {
        if (++count >= 100) { count = 0; return true; }
        return false;
    }
};
} // namespace aion::m1::l3_5
