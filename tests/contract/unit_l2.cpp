#include "aion/m1/l2/l2.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <algorithm>
#include <vector>
using namespace aion::m1::l2;

int main() {
    // EMA: converges to 5.0
    EMA ema(0.2f, 0.0f);
    for (int i = 0; i < 200; ++i) ema.update(5.0f);
    assert(std::fabs(ema.value() - 5.0f) < 1e-3f);

    // P2 median on uniform 0..99 should be near 50
    P2Quantile p2(0.5f);
    for (int i = 0; i < 1000; ++i) p2.update(float(i % 100));
    assert(p2.ready());
    float est = p2.value();
    assert(est > 35.0f && est < 65.0f);

    // KSGkNN: kth-NN distance finite, monotone-ish on two clusters
    KSGkNN k;
    for (int i = 0; i < 50; ++i) {
        KSGkNN::Sample s{};
        for (size_t d = 0; d < KSGkNN::DIM; ++d) s.x[d] = float(i) * 0.1f;
        s.y = 0.0f;
        assert(k.add(s));
    }
    float m3 = k.mean_kth_nn_distance(3);
    float m10 = k.mean_kth_nn_distance(10);
    assert(std::isfinite(m3) && m3 > 0.0f);
    assert(std::isfinite(m10) && m10 >= m3);

    std::puts("unit_l2 OK");
    return 0;
}
