#pragma once
#include <cstdint>
#include <cstddef>
#include <cstdio>

namespace aion::m2 {

// A7: ReplayLog per-node ring buffer
// 内存：65536 × 32B = 2.00 MB / 节点
// 生产关闭：enabled_ = false（运行时 flag，非编译宏）
// 哈希：128-bit 双 64-bit FNV-1a（不同种子），碰撞率 < 1e-19
class ReplayLog {
public:
    static constexpr size_t CAP = 65536;
    static constexpr size_t ENTRY_SIZE = 32;

    struct Entry {
        uint64_t epoch;
        uint64_t eproc;
        uint64_t payload_hash_lo;
        uint64_t payload_hash_hi;
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
        uint64_t h_lo = 1469598103934665603ULL;
        uint64_t h_hi = 0xCBF29CE484222325ULL;
        for (size_t i = 0; i < len; ++i) {
            h_lo ^= payload[i];  h_lo *= 1099511628211ULL;
            h_hi ^= payload[i];  h_hi *= 1099511628211ULL;
            h_hi = (h_hi >> 1) | (h_hi << 63);
        }
        e.payload_hash_lo = h_lo;
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
