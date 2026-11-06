#pragma once
#include <cstddef>
namespace aion::m1::l1 {
template <typename T, size_t N>
class FixedVec {
public:
    static constexpr size_t capacity() noexcept { return N; }
    size_t size() const noexcept { return size_; }
    bool   empty() const noexcept { return size_ == 0; }
    bool   full() const noexcept { return size_ >= N; }
    T*       data()       noexcept { return storage_; }
    const T* data() const noexcept { return storage_; }
    T&       operator[](size_t i)       noexcept { return storage_[i]; }
    const T& operator[](size_t i) const noexcept { return storage_[i]; }

    bool push_back(const T& v) noexcept {
        if (size_ >= N) return false;
        storage_[size_++] = v;
        return true;
    }
    void pop_back() noexcept { if (size_ > 0) --size_; }
    void clear() noexcept { size_ = 0; }
private:
    T storage_[N ? N : 1]{};
    size_t size_{0};
};
} // namespace aion::m1::l1
