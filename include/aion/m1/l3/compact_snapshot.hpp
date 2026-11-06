#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>

namespace aion::m1::l3 {
static constexpr size_t SNAP_FEAT = 12;

struct alignas(16) CompactSnapshot {
    uint32_t node_id;
    uint32_t n_feat;
    uint64_t epoch;
    float    feat[SNAP_FEAT];
    uint64_t checksum;
    uint64_t seq;
};

static_assert(sizeof(CompactSnapshot) == 80, "must be 80B");
static_assert(alignof(CompactSnapshot) == 16, "must be alignas(16)");
static_assert(offsetof(CompactSnapshot, feat) == 16, "feat at 16");
static_assert(offsetof(CompactSnapshot, checksum) == 64, "checksum at 64");
static_assert(offsetof(CompactSnapshot, seq) == 72, "seq at 72");

inline uint64_t snap_checksum(const CompactSnapshot& s) noexcept {
    uint64_t h = 1469598103934665603ULL;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&s);
    for (size_t i = 0; i < 64; ++i) { h ^= p[i]; h *= 1099511628211ULL; }
    return h;
}

inline bool has_16b_lock_free() noexcept { return __atomic_always_lock_free(16, nullptr); }

// A3 修复：checksum 在第 5 段构造时计算，覆盖前 4 段已发布数据
inline void store_compact(CompactSnapshot& dst, const CompactSnapshot& src) noexcept {
    CompactSnapshot tmp = src;
    tmp.seq = dst.seq + 1;
    tmp.checksum = 0; // 暂时留空
    char* d = reinterpret_cast<char*>(&dst);
    const char* s = reinterpret_cast<const char*>(&tmp);
    // 先写前 4 段
    for (int seg = 0; seg < 4; ++seg) {
        __int128 v;
        std::memcpy(&v, s + seg * 16, 16);
        __atomic_store(reinterpret_cast<__int128*>(d + seg * 16), &v, __ATOMIC_RELEASE);
    }
    // 此时写线程可能被抢占。读线程如果此刻读取，checksum 是旧的/0，必然校验失败 -> 返回 false
    // 在写第 5 段前，计算 checksum
    tmp.checksum = snap_checksum(tmp);
    __int128 v;
    std::memcpy(&v, s + 64, 16);
    __atomic_store(reinterpret_cast<__int128*>(d + 64), &v, __ATOMIC_RELEASE);
}

inline bool load_compact(const CompactSnapshot& src, CompactSnapshot& dst) noexcept {
    const char* s = reinterpret_cast<const char*>(&src);
    __int128 tail;
    __atomic_load(reinterpret_cast<const __int128*>(s + 64), &tail, __ATOMIC_ACQUIRE);
    __atomic_thread_fence(__ATOMIC_ACQUIRE);
    char buf[64];
    for (int seg = 0; seg < 4; ++seg) {
        __int128 v;
        __atomic_load(reinterpret_cast<const __int128*>(s + seg * 16), &v, __ATOMIC_ACQUIRE);
        std::memcpy(buf + seg * 16, &v, 16);
    }
    std::memcpy(reinterpret_cast<char*>(&dst), buf, 64);
    std::memcpy(reinterpret_cast<char*>(&dst) + 64, &tail, 16);
    return snap_checksum(dst) == dst.checksum;
}
} // namespace aion::m1::l3
