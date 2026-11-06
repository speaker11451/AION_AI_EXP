#pragma once
#include <array>
#include <atomic>
#include <cstddef>
namespace aion::m1::l3 {
template <typename T, size_t N>
class SPSCQueue {
    static_assert(N >= 2, "N >= 2");
    static_assert((N & (N - 1)) == 0, "N must be power of 2");
public:
    SPSCQueue() = default;
    SPSCQueue(const SPSCQueue&) = delete;
    SPSCQueue& operator=(const SPSCQueue&) = delete;

    bool push(const T& v) noexcept {
        size_t h = head_.load(std::memory_order_relaxed);
        size_t t = tail_.load(std::memory_order_acquire);
        if (h - t >= N) return false;
        buf_[h & (N - 1)] = v;
        head_.store(h + 1, std::memory_order_release);
        return true;
    }
    bool pop(T& out) noexcept {
        size_t t = tail_.load(std::memory_order_relaxed);
        size_t h = head_.load(std::memory_order_acquire);
        if (h == t) return false;
        out = buf_[t & (N - 1)];
        tail_.store(t + 1, std::memory_order_release);
        return true;
    }
    size_t size() const noexcept {
        size_t h = head_.load(std::memory_order_acquire);
        size_t t = tail_.load(std::memory_order_acquire);
        return h - t;
    }
    bool empty() const noexcept { return size() == 0; }
    static constexpr size_t capacity() noexcept { return N; }
private:
    std::array<T, N> buf_{};
    alignas(64) std::atomic<size_t> head_{0};
    alignas(64) std::atomic<size_t> tail_{0};
};
} // namespace aion::m1::l3
