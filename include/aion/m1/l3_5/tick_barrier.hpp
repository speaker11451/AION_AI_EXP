#pragma once
#include "aion/m1/l0/time_point.hpp"
namespace aion::m1::l3_5 {
using namespace aion::m1::l0;
struct TickBarrier {
    bool frozen{false};
    int reason{0};
    bool try_freeze(int r, TimePoint) noexcept {
        if (frozen) return false;
        frozen = true; reason = r; return true;
    }
    void try_thaw(TimePoint, bool ready) noexcept {
        if (frozen && ready) frozen = false;
    }
};
} // namespace aion::m1::l3_5
