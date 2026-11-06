#pragma once
#include <array>
#include <cmath>
#include <cstddef>
#include "aion/m1/l1/fixed_vec.hpp"
namespace aion::m1::l2 {

// KSG kNN mutual-information estimator: M1 delivers interface + basic
// brute-force kth-NN mean distance. Real KSG MI formula / 512x5 optimizations
// are deferred to M2 (see M1 scope: "KSGkNN performance not gating").
struct KSGkNN {
    static constexpr size_t DIM = 5;
    static constexpr size_t MAX = 512;
    struct Sample { std::array<float, DIM> x; float y; };

    l1::FixedVec<Sample, MAX> samples;

    bool add(const Sample& s) noexcept { return samples.push_back(s); }
    void clear() noexcept { samples.clear(); }
    size_t size() const noexcept { return samples.size(); }

    // Mean over all points of distance to k-th nearest neighbour in full DIM space.
    float mean_kth_nn_distance(size_t k) const noexcept {
        if (samples.size() < 2 || k == 0) return 0.0f;
        float sum = 0.0f; size_t cnt = 0;
        l1::FixedVec<float, MAX> ds;
        for (size_t i = 0; i < samples.size(); ++i) {
            ds.clear();
            for (size_t j = 0; j < samples.size(); ++j) {
                if (i == j) continue;
                float d2 = 0.0f;
                for (size_t d = 0; d < DIM; ++d) { float e = samples[i].x[d] - samples[j].x[d]; d2 += e * e; }
                ds.push_back(std::sqrt(d2));
            }
            if (ds.size() < k) continue;
            // kth smallest (1-indexed), k <= MAX/2 for M1
            l1::FixedVec<float, MAX> tmp = ds;
            std::sort(tmp.data(), tmp.data() + tmp.size());
            sum += tmp[k - 1];
            ++cnt;
        }
        return cnt ? sum / float(cnt) : 0.0f;
    }

    // MI estimate: M1 returns 0 (stub). Full KSG MI deferred to M2.
    float estimate_mi(size_t k) const noexcept { (void)k; return 0.0f; }
};
} // namespace aion::m1::l2
