#pragma once
#include <array>
#include <algorithm>
#include <cstddef>
namespace aion::m1::l2 {

// P² quantile estimator (Jain & Chlamtac 1985)
// 5 markers, single pass, fixed state, no heap.
class P2Quantile {
public:
    explicit P2Quantile(float q = 0.5f) noexcept : q_(q < 0 ? 0 : (q > 1 ? 1 : q)) {
        // dn_ 是每次样本后 np_ 的增量
        dn_[0] = 0.0f;
        dn_[1] = q_ / 2.0f;
        dn_[2] = q_;
        dn_[3] = (1.0f + q_) / 2.0f;
        dn_[4] = 1.0f;
    }

    float update(float x) noexcept {
        if (!init_) {
            buf_[fill_++] = x;
            if (fill_ == 5) {
                std::sort(buf_.begin(), buf_.end());
                for (int i = 0; i < 5; ++i) { h_[i] = buf_[i]; n_[i] = float(i + 1); }
                np_[0] = 1.0f;
                np_[1] = 1.0f + 2.0f * q_;
                np_[2] = 1.0f + 4.0f * q_;
                np_[3] = 3.0f + 2.0f * q_;
                np_[4] = 5.0f;
                init_ = true;
            }
            return init_ ? h_[2] : buf_[0];
        }

        // 1) 找到 x 落在哪个区间
        int k;
        if      (x < h_[0]) { h_[0] = x; k = 0; }
        else if (x < h_[1]) { k = 0; }
        else if (x < h_[2]) { k = 1; }
        else if (x < h_[3]) { k = 2; }
        else if (x <= h_[4]) { k = 3; }
        else                 { h_[4] = x; k = 3; }

        // 2) 更新实际 rank n_
        for (int i = k + 1; i < 5; ++i) n_[i] += 1.0f;

        // 3) 更新目标 rank np_（原代码漏了这一步）
        for (int i = 0; i < 5; ++i) np_[i] += dn_[i];

        // 4) 调整中间 3 个 marker（h_[1..3]）
        for (int i = 1; i < 4; ++i) {
            float d = np_[i] - n_[i];
            int di = (d >= 0.0f) ? 1 : -1;
            bool cond = (d >= 1.0f && n_[i + 1] - n_[i] > 1.0f) ||
                        (d <= -1.0f && n_[i - 1] - n_[i] < -1.0f);
            if (!cond) continue;
            float q_new = h_[i] + float(di) * (h_[i + di] - h_[i]) / (n_[i + di] - n_[i]);
            if (h_[i - 1] < q_new && q_new < h_[i + 1]) h_[i] = q_new;
            else h_[i] = q_new;  // 边界情况
            n_[i] += float(di);
        }
        return h_[2];
    }

    float value() const noexcept { return init_ ? h_[2] : 0.0f; }
    float q() const noexcept { return q_; }
    bool  ready() const noexcept { return init_; }

private:
    float q_{0.5f};
    std::array<float, 5> h_{};
    std::array<float, 5> n_{};
    std::array<float, 5> np_{};
    std::array<float, 5> dn_{};
    std::array<float, 5> buf_{};
    int   fill_{0};
    bool  init_{false};
};
} // namespace aion::m1::l2
