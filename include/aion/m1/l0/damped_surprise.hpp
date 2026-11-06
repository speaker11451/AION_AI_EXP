#pragma once
namespace aion::m1::l0 {
struct DampedSurprise {
    float raw_nats{0.0f};
    float confidence{0.0f}; // 0.0 = low confidence, 1.0 = high
    DampedSurprise() = delete;
    static DampedSurprise from_low_confidence(float raw) noexcept {
        return DampedSurprise(raw, 0.0f, Tag{});
    }
    static DampedSurprise from_high_confidence(float raw) noexcept {
        return DampedSurprise(raw, 1.0f, Tag{});
    }
private:
    struct Tag {};
    DampedSurprise(float r, float c, Tag) noexcept : raw_nats(r), confidence(c) {}
};
} // namespace aion::m1::l0
