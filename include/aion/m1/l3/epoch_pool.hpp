#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include "aion/m1/l3/compact_snapshot.hpp"

namespace aion::m1::l3 {

// M1 scope: fixed history depth 16 per node, single-writer per history slot,
// append_history writes CompactSnapshot only. Rotate advances write cursor.
class EpochPool {
public:
    static constexpr size_t MAX_NODES = 128;
    static constexpr size_t DEPTH    = 16;

    struct Slot {
        CompactSnapshot snap{};
        std::atomic<int>  refcount{0};
        std::atomic<bool> valid{false};
    };

    Slot* acquire(uint32_t node_id) noexcept {
        if (node_id >= MAX_NODES) return nullptr;
        auto& h = histories_[node_id];
        Slot& s = h.slots[h.write_idx % DEPTH];
        s.refcount.fetch_add(1, std::memory_order_acq_rel);
        return &s;
    }
    void release(Slot* s) noexcept {
        if (!s) return;
        s->refcount.fetch_sub(1, std::memory_order_acq_rel);
    }
    void rotate(uint32_t node_id) noexcept {
        if (node_id >= MAX_NODES) return;
        auto& h = histories_[node_id];
        h.write_idx = (h.write_idx + 1) % DEPTH;
    }

    // append_history writes only CompactSnapshot into the current slot,
    // stores raw features into snap.feat[0..n-1], and marks it valid.
    void append_history(uint32_t node_id, uint64_t epoch,
                        const float* feat, size_t n) noexcept {
        if (node_id >= MAX_NODES) return;
        auto& h = histories_[node_id];
        Slot& s = h.slots[h.write_idx % DEPTH];
        CompactSnapshot snap{};
        snap.node_id = node_id;
        snap.epoch   = epoch;
        snap.n_feat  = uint32_t(n > SNAP_FEAT ? SNAP_FEAT : n);
        for (size_t i = 0; i < snap.n_feat; ++i) snap.feat[i] = feat ? feat[i] : 0.0f;
        s.valid.store(false, std::memory_order_release);
        store_compact(s.snap, snap);
        s.valid.store(true, std::memory_order_release);
    }

    // Linear scan of history; returns newest slot with snap.epoch <= target.
    const CompactSnapshot* find_at_or_before(uint32_t node_id, uint64_t target_epoch) const noexcept {
        if (node_id >= MAX_NODES) return nullptr;
        const auto& h = histories_[node_id];
        const CompactSnapshot* best = nullptr;
        uint64_t best_epoch = 0;
        for (size_t i = 0; i < DEPTH; ++i) {
            const Slot& s = h.slots[i];
            if (!s.valid.load(std::memory_order_acquire)) continue;
            CompactSnapshot tmp{};
            if (!load_compact(s.snap, tmp)) continue;
            if (tmp.epoch <= target_epoch && (!best || tmp.epoch > best_epoch)) {
                // copy into a static thread_local buffer so we can return a pointer
                thread_local CompactSnapshot out;
                out = tmp;
                best = &out;
                best_epoch = tmp.epoch;
            }
        }
        return best;
    }
private:
    struct History {
        Slot slots[DEPTH]{};
        uint32_t write_idx{0};
    };
    std::array<History, MAX_NODES> histories_{};
};

} // namespace aion::m1::l3
