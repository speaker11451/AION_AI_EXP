#include "aion/m1/l2/l2.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
#include <random>
#include <algorithm>
using namespace aion::m1::l2;

int main() {
    // EMA 收敛
    EMA ema(0.2f, 0.0f);
    for (int i = 0; i < 200; ++i) ema.update(5.0f);
    assert(std::fabs(ema.value() - 5.0f) < 1e-3f);

    // P2 median：均匀随机输入 [0,100]，应接近 50
    P2Quantile p2(0.5f);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> dist(0.0f, 100.0f);
    for (int i = 0; i < 20000; ++i) p2.update(dist(rng));
    assert(p2.ready());
    float est = p2.value();
    printf("P2 median estimate = %.2f (真值 50)\n", est);
    assert(est > 35.0f && est < 65.0f);

    // KSGkNN 基本形状
    KSGkNN k;
    for (int i = 0; i < 50; ++i) {
        KSGkNN::Sample s{};
        for (size_t d = 0; d < KSGkNN::DIM; ++d) s.x[d] = float(i) * 0.1f;
        s.y = 0.0f;
        assert(k.add(s));
    }
    float m3 = k.mean_kth_nn_distance(3);
    float m10 = k.mean_kth_nn_distance(10);
    printf("k=3 mean dist = %.4f, k=10 mean dist = %.4f\n", m3, m10);
    assert(std::isfinite(m3) && m3 > 0.0f);
    assert(std::isfinite(m10) && m10 >= m3);

    std::puts("unit_l2 OK");
    return 0;
}
