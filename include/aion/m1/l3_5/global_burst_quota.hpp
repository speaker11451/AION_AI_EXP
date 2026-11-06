#pragma once
#include <cstdint>
#include <atomic>
namespace aion::m1::l3_5 {
struct GlobalBurstQuota {
    static constexpr uint32_t MAX_WORKERS = 8;
    static constexpr uint32_t TOTAL_QUOTA = 16; // 2 * NWORKERS
    static constexpr uint32_t PER_WORKER_INIT = 2;
    static constexpr uint32_t PER_WORKER_MAX = 4;

    std::atomic<uint32_t> per_worker_quota[MAX_WORKERS];
    std::atomic<uint32_t> used[MAX_WORKERS];

    GlobalBurstQuota() noexcept {
        for (uint32_t i = 0; i < MAX_WORKERS; ++i) {
            per_worker_quota[i].store(PER_WORKER_INIT);
            used[i].store(0);
        }
    }

    // 热路径，无锁
    bool try_acquire(uint32_t worker_id) noexcept {
        if (worker_id >= MAX_WORKERS) return false;
        uint32_t u = used[worker_id].load(std::memory_order_relaxed);
        if (u < per_worker_quota[worker_id].load(std::memory_order_relaxed)) {
            used[worker_id].fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    // 慢路径（50ms 一次），原子再平衡
    void rebalance() noexcept {
        uint32_t total_used = 0;
        for (uint32_t i = 0; i < MAX_WORKERS; ++i) total_used += used[i].load();
        if (total_used == 0) return;

        for (uint32_t i = 0; i < MAX_WORKERS; ++i) {
            uint32_t u = used[i].exchange(0);
            uint32_t q = per_worker_quota[i].load();
            // 简单规则：如果某 worker 经常用满，给它加 1（上限 4）；如果长期不用，减 1（下限 1）
            if (u >= q) {
                if (q < PER_WORKER_MAX) per_worker_quota[i].fetch_add(1);
            } else if (u == 0) {
                if (q > 1) per_worker_quota[i].fetch_sub(1);
            }
        }
    }
};
} // namespace aion::m1::l3_5
