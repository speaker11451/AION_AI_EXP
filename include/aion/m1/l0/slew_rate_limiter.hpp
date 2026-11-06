#pragma once
#include <cmath>
namespace aion::m1::l0 {
struct SlewRateLimiter {
    float max_delta_per_step{0.0f};
    float current{0.0f};
    SlewRateLimiter() = default;
    SlewRateLimiter(float max_d, float init) noexcept : max_delta_per_step(max_d), current(init) {}
    float step(float target) noexcept {
        float d = target - current;
        if (d >  max_delta_per_step) d =  max_delta_per_step;
        if (d < -max_delta_per_step) d = -max_delta_per_step;
        current += d;
        return current;
    }
};
} // namespace aion::m1::l0
