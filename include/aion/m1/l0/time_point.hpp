#pragma once
#include <cstdint>
#include <cmath>
namespace aion::m1::l0 {

struct TimePoint {
    uint64_t us{0};
    friend constexpr bool operator==(TimePoint a, TimePoint b) noexcept { return a.us == b.us; }
    friend constexpr bool operator<(TimePoint a, TimePoint b) noexcept { return a.us < b.us; }
};

struct LogicalEpoch {
    uint64_t v{0};
    constexpr LogicalEpoch next() const noexcept { return LogicalEpoch{v + 1}; }
    friend constexpr bool operator==(LogicalEpoch a, LogicalEpoch b) noexcept { return a.v == b.v; }
};

struct LogTau {
    static constexpr float MIN_US = 1.0f;
    static constexpr float MAX_US = 1.0e9f;
    float log_us{0.0f};

    static LogTau from_us(float us) noexcept {
        if (!std::isfinite(us) || us < MIN_US) us = MIN_US;
        if (us > MAX_US) us = MAX_US;
        return LogTau{ std::log(us) };
    }
    float to_us() const noexcept { return std::exp(log_us); }
    LogTau scaled_by(float k) const noexcept { return from_us(to_us() * k); }
};

} // namespace aion::m1::l0
