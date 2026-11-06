#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>

namespace aion::m2 {

// A7: ReplayLog per-node ring buffer
// 内存：65536 × 32B = 2.00 MB / 节点
// 哈希：128-bit 双家族
//   - hash_lo: FNV-1a 64（offset basis 0xCBF29CE484222325, prime 0x100000001b3）
//   - hash_hi: splitmix64 混入每条 payload byte（完全不同算法家族）
// 独立性证明：两个 hash 不同 offset basis / 不同乘子 / 不同混合模式，
// 生日界 2^64（远大于 65536 条目，碰撞可忽略）
class ReplayLog {
public:
    static constexpr size_t CAP = 65536;
    static constexpr size_t ENTRY_SIZE = 32;

    struct Entry {
        uint64_t epoch;
        uint64_t eproc;
        uint64_t payload_hash_lo;   // FNV-1a 64
        uint64_t payload_hash_hi;   // SplitMix64 家族
    };

    ReplayLog() noexcept = default;

    bool enable(bool on) noexcept {
        enabled_ = on;
        fprintf(stderr, "[A7] ReplayLog %s (per-node, 65536 entries, 2.00MB/node)\n",
                on ? "ENABLED (test mode)" : "DISABLED (production)");
        return enabled_;
    }
    bool is_enabled() const noexcept { return enabled_; }

    bool push(uint64_t epoch, uint64_t eproc,
              const uint8_t* payload, size_t len) noexcept {
        if (!enabled_) return false;
        Entry& e = entries_[head_ % CAP];
        e.epoch = epoch;
        e.eproc = eproc;

        // hash_lo: 标准 FNV-1a 64
        uint64_t h_lo = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; ++i) {
            h_lo ^= payload[i];
            h_lo *= 0x100000001b3ULL;
        }
        e.payload_hash_lo = h_lo;

        // hash_hi: splitmix64 混入（不同算法家族，offset basis 不同）
        uint64_t h_hi = 0x9E3779B97F4A7C15ULL ^ (len << 32);
        for (size_t i = 0; i < len; ++i) {
            h_hi += payload[i];
            h_hi = (h_hi ^ (h_hi >> 30)) * 0xBF58476D1CE4E5B9ULL;
            h_hi = (h_hi ^ (h_hi >> 27)) * 0x94D049BB133111EBULL;
            h_hi = h_hi ^ (h_hi >> 31);
        }
        e.payload_hash_hi = h_hi;

        ++head_;
        return true;
    }

    const Entry* at(size_t i) const noexcept {
        if (i >= size()) return nullptr;
        return &entries_[(tail_ + i) % CAP];
    }
    size_t size() const noexcept { return head_ - tail_; }
    void clear() noexcept { head_ = tail_ = 0; }

private:
    Entry entries_[CAP]{};
    uint64_t head_{0};
    uint64_t tail_{0};
    bool enabled_{false};
};

} // namespace aion::m2
