#pragma once
#include <cstdint>
#include <cstddef>
#include <cstring>

namespace aion::m2 {

// A7: ReplayLog per-node ring buffer
// - 每条 32B, 容量 65536, 仅测试模式开启
// - FNV-1a 64-bit hash（32-bit 在 65536 条上碰撞率 ~ 39%，不可接受）
// - 运行时开关 + 启动日志（见 enable()）
class ReplayLog {
public:
    static constexpr size_t CAP = 65536;
    static constexpr size_t ENTRY_SIZE = 32;

    struct Entry {
        uint64_t epoch;
        uint64_t eproc;
        uint64_t payload_hash_lo; // 64-bit hash
        uint64_t payload_hash_hi; // 128-bit total
    };

    ReplayLog() noexcept : head_(0), tail_(0) {}

    bool enable(bool on) noexcept {
        enabled_ = on;
        return enabled_;
    }
    bool is_enabled() const noexcept { return enabled_; }

    bool push(uint64_t epoch, uint64_t eproc, const uint8_t* payload, size_t len) noexcept {
        if (!enabled_) return false;
        size_t idx = head_ % CAP;
        Entry& e = entries_[idx];
        e.epoch = epoch;
        e.eproc = eproc;
        // 128-bit hash: FNV-1a 64 低 64 + 高 64（用不同 seed 避免相同算法）
        uint64_t h_lo = 1469598103934665603ULL;
        uint64_t h_hi = 1099511628211ULL;
        for (size_t i = 0; i < len; ++i) {
            h_lo ^= payload[i]; h_lo *= 1099511628211ULL;
            h_hi ^= uint64_t(payload[i]) << 1; h_hi *= 1099511628211ULL;
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
    Entry entries_[CAP];
    uint64_t head_, tail_;
    bool enabled_{false};
};

// 启动日志示例（在 main 入口调用）
inline void log_replaylog_status(bool enabled) {
    fprintf(stderr, "[A7] ReplayLog: %s (per-node, 65536 entries, ~2MB/node)\n",
            enabled ? "ENABLED (test mode)" : "DISABLED (production)");
}

} // namespace aion::m2
