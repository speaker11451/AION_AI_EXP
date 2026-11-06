#pragma once
#include <cmath>
namespace aion::m1::l0 {
struct LogDomainFloat {
    static constexpr float MIN_VAL = 1e-30f;
    float log_val{0.0f};
    static LogDomainFloat from_value(float x) noexcept {
        if (!std::isfinite(x) || x < MIN_VAL) x = MIN_VAL;
        return LogDomainFloat{ std::log(x) };
    }
    float to_value() const noexcept { return std::exp(log_val); }
};
} // namespace aion::m1::l0
