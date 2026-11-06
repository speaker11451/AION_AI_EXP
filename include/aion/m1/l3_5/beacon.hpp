#pragma once
#include "aion/m1/l0/l0.hpp"
namespace aion::m1::l3_5 {
using namespace aion::m1::l0;
struct BeaconPhase {
    TimePoint offset;
    static BeaconPhase init(uint32_t node_id) noexcept {
        SplitMix64 rng(node_id);
        return BeaconPhase{ TimePoint{ uint64_t(rng.next() % 3000) } }; // 3ms window
    }
    TimePoint reserve(TimePoint now) const noexcept {
        return TimePoint{ now.us + offset.us };
    }
};
struct BeaconProbability {
    float p{1.0f};
    bool should_send(SplitMix64& rng) noexcept {
        return rng.next_float() < p;
    }
    void on_ack() noexcept { p = (p < 0.1f) ? p * 1.1f : p; } // 简单衰减恢复
};
} // namespace aion::m1::l3_5
