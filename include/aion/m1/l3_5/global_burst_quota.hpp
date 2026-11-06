#pragma once
#include <cstdint>
#include <atomic>
namespace aion::m1::l3_5 {
struct GlobalBurstQuota {
    static constexpr uint32_t MAX_WORKERS = 8;
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

    bool try_acquire(uint32_t worker_id) noexcept {
        if (worker_id >= MAX_WORKERS) return false;
        uint32_t u = used[worker_id].load(std::memory_order_relaxed);
        if (u < per_worker_quota[worker_id].load(std::memory_order_relaxed)) {
            used[worker_id].fetch_add(1, std::memory_order_relaxed);
            return true;
        }
        return false;
    }

    // 慢路径再平衡：只增加超限 worker 的配额，绝不削减其他 worker 的配额，
    // 避免 1 个 worker 超限就饿死其他 7 个 worker。
    void rebalance() noexcept {
        for (uint32_t i = 0; i < MAX_WORKERS; ++i) {
            uint32_t u = used[i].exchange(0);
            uint32_t q = per_worker_quota[i].load();
            if (u >= q && q < PER_WORKER_MAX) {
                per_worker_quota[i].fetch_add(1);
            }
        }
    }
};
} // namespace aion::m1::l3_5
