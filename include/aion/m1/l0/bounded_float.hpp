#pragma once
#include <cmath>
namespace aion::m1::l0 {

struct SurpriseNatsTag;
struct IntensiveRateTag;

template <float MinV, float MaxV, typename Tag = void>
struct BoundedFloat {
    static constexpr float MIN = MinV;
    static constexpr float MAX = MaxV;
    float v;

    explicit BoundedFloat(float x) noexcept {
        if (!std::isfinite(x)) x = (x < 0.0f) ? MinV : MaxV;
        if (x < MinV) x = MinV;
        if (x > MaxV) x = MaxV;
        v = x;
    }
    BoundedFloat& operator+=(float d) noexcept {
        float n = v + d;
        if (!std::isfinite(n)) n = (d > 0.0f) ? MaxV : MinV;
        if (n < MinV) n = MinV;
        if (n > MaxV) n = MaxV;
        v = n;
        return *this;
    }
};

// 同一 Tag 同一区间才允许相加；跨 Tag 无定义 -> 编译失败
template <float MinV, float MaxV, typename Tag>
inline BoundedFloat<MinV,MaxV,Tag> operator+(BoundedFloat<MinV,MaxV,Tag> a,
                                              BoundedFloat<MinV,MaxV,Tag> b) noexcept {
    return BoundedFloat<MinV,MaxV,Tag>(a.v + b.v);
}

using SurpriseNats   = BoundedFloat<0.0f, 100.0f, SurpriseNatsTag>;
using IntensiveRate  = BoundedFloat<0.0f, 1000.0f, IntensiveRateTag>;

} // namespace aion::m1::l0
