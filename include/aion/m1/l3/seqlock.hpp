#pragma once
#include <atomic>
#include <cstdint>
namespace aion::m1::l3 {
template <typename T>
class Seqlock {
public:
    Seqlock() = default;
    Seqlock(const Seqlock&) = delete;
    Seqlock& operator=(const Seqlock&) = delete;

    void write(const T& v) noexcept {
        uint64_t s = seq_.load(std::memory_order_relaxed);
        seq_.store(s + 1, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_release);
        data_ = v;
        seq_.store(s + 2, std::memory_order_release);
    }
    bool read(T& out) const noexcept {
        uint64_t s1 = seq_.load(std::memory_order_acquire);
        if (s1 & 1u) return false;
        std::atomic_thread_fence(std::memory_order_acquire);
        T tmp = data_;
        std::atomic_thread_fence(std::memory_order_acquire);
        uint64_t s2 = seq_.load(std::memory_order_acquire);
        if (s1 != s2) return false;
        out = tmp;
        return true;
    }

#ifdef AION_TEST_ACCESS
    // 仅用于 TSan 注入测试：绕过 read() 直接返回 data_ 引用，
    // 让数据竞争暴露在 Seqlock::read 之外（从而避开 tsan.supp 白名单）
    const T& raw_data() const noexcept { return data_; }
#endif

private:
    alignas(64) std::atomic<uint64_t> seq_{0};
    alignas(64) T data_{};
};
} // namespace aion::m1::l3
