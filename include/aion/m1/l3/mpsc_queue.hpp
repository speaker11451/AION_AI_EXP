#pragma once
#include <array>
#include <atomic>
#include <cstddef>
#include <cstdint>
namespace aion::m1::l3 {

// Vyukov bounded MPMC queue; M1 uses it in MPSC configuration (many producers,
// single consumer). Lock-free, no heap.
template <typename T, size_t N>
class MPSCQueue {
    static_assert(N >= 2, "N >= 2");
    static_assert((N & (N - 1)) == 0, "N must be power of 2");
public:
    MPSCQueue() noexcept {
        for (size_t i = 0; i < N; ++i) cells_[i].seq.store(i, std::memory_order_relaxed);
    }
    MPSCQueue(const MPSCQueue&) = delete;
    MPSCQueue& operator=(const MPSCQueue&) = delete;

    bool push(const T& v) noexcept {
        size_t pos = enq_.load(std::memory_order_relaxed);
        for (;;) {
            Cell& c = cells_[pos & (N - 1)];
            size_t seq = c.seq.load(std::memory_order_acquire);
            intptr_t diff = intptr_t(seq) - intptr_t(pos);
            if (diff == 0) {
                if (enq_.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_relaxed, std::memory_order_relaxed)) {
                    c.data = v;
                    c.seq.store(pos + 1, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = enq_.load(std::memory_order_relaxed);
            }
        }
    }
    bool pop(T& out) noexcept {
        size_t pos = deq_.load(std::memory_order_relaxed);
        for (;;) {
            Cell& c = cells_[pos & (N - 1)];
            size_t seq = c.seq.load(std::memory_order_acquire);
            intptr_t diff = intptr_t(seq) - intptr_t(pos + 1);
            if (diff == 0) {
                if (deq_.compare_exchange_weak(pos, pos + 1,
                        std::memory_order_relaxed, std::memory_order_relaxed)) {
                    out = c.data;
                    c.seq.store(pos + N, std::memory_order_release);
                    return true;
                }
            } else if (diff < 0) {
                return false;
            } else {
                pos = deq_.load(std::memory_order_relaxed);
            }
        }
    }
    static constexpr size_t capacity() noexcept { return N; }
private:
    struct Cell {
        std::atomic<size_t> seq;
        T data{};
        Cell() noexcept : seq(0) {}
    };
    std::array<Cell, N> cells_;
    alignas(64) std::atomic<size_t> enq_{0};
    alignas(64) std::atomic<size_t> deq_{0};
};
} // namespace aion::m1::l3
