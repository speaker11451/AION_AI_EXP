#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <algorithm>
namespace aion::m1::l3 {
template <size_t MAX_NODES = 64>
struct VectorClockT {
    struct Slot { uint32_t node; uint64_t epoch; };
    std::array<Slot, MAX_NODES> slots{};
    uint32_t n_slots{0};
    uint64_t summary_min{ UINT64_MAX }; // min epoch of evicted

    int compare(uint32_t from, uint64_t from_epoch) const noexcept {
        for (uint32_t i = 0; i < n_slots; ++i) {
            if (slots[i].node == from) {
                if (slots[i].epoch == from_epoch) return 0;
                return from_epoch < slots[i].epoch ? 1 : -1;
            }
        }
        if (from_epoch < summary_min) return 2; // unknown, conservative concurrent (D1 fix)
        return 2; // unknown, conservative concurrent
    }

    void merge(uint32_t from, uint64_t from_epoch) noexcept {
        for (uint32_t i = 0; i < n_slots; ++i) {
            if (slots[i].node == from) {
                if (from_epoch > slots[i].epoch) slots[i].epoch = from_epoch;
                return;
            }
        }
        if (n_slots < MAX_NODES) { slots[n_slots++] = {from, from_epoch}; return; }
        uint32_t min_i = 0;
        for (uint32_t i = 1; i < n_slots; ++i) if (slots[i].epoch < slots[min_i].epoch) min_i = i;
        summary_min = std::min(summary_min, slots[min_i].epoch);
        slots[min_i] = {from, from_epoch};
    }
};
using VectorClock = VectorClockT<64>;
} // namespace aion::m1::l3
