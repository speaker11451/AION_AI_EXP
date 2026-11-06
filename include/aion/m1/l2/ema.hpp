#pragma once
namespace aion::m1::l2 {
struct EMA {
    float alpha{0.1f};
    float state{0.0f};
    bool  initialized{false};
    EMA() = default;
    explicit EMA(float a, float init = 0.0f) noexcept : alpha(a), state(init) {}
    float update(float x) noexcept {
        if (!initialized) { state = x; initialized = true; return state; }
        state = alpha * x + (1.0f - alpha) * state;
        return state;
    }
    float value() const noexcept { return state; }
    void reset() noexcept { state = 0.0f; initialized = false; }
};
} // namespace aion::m1::l2
