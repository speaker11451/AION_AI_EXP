#pragma once
#include <cstddef>
namespace aion::m1::l1 {
template <typename T, size_t N>
class RingBuffer {
    static_assert(N > 0, "N must be > 0");
    static_assert((N & (N - 1)) == 0, "N must be power of 2");
public:
    static constexpr size_t capacity() noexcept { return N; }
    bool empty() const noexcept { return head_ == tail_; }
    bool full()  const noexcept { return (head_ - tail_) == N; }
    size_t size() const noexcept { return head_ - tail_; }

    bool push(const T& v) noexcept {
        if (full()) return false;
        buf_[head_ & (N - 1)] = v;
        ++head_;
        return true;
    }
    bool pop(T& out) noexcept {
        if (empty()) return false;
        out = buf_[tail_ & (N - 1)];
        ++tail_;
        return true;
    }
    void clear() noexcept { head_ = tail_ = 0; }
private:
    T buf_[N]{};
    size_t head_{0};
    size_t tail_{0};
};
} // namespace aion::m1::l1
