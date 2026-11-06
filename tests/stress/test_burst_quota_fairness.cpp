#include "aion/m1/l3_5/global_burst_quota.hpp"
#include <cstdio>
#include <cassert>
using namespace aion::m1::l3_5;

int main() {
    // 场景 A：基准（8 worker 均匀请求）
    uint32_t baseline[8] = {0};
    {
        GlobalBurstQuota quota;
        for (int round = 0; round < 1000; ++round) {
            for (int w = 0; w < 8; ++w)
                if (quota.try_acquire(w)) baseline[w]++;
            if (round % 50 == 49) quota.rebalance();
        }
    }

    // 场景 B：worker 0 每轮尝试 10 次（超限），其他 7 worker 每轮 1 次
    uint32_t overload[8] = {0};
    {
        GlobalBurstQuota quota;
        for (int round = 0; round < 1000; ++round) {
            for (int k = 0; k < 10; ++k)
                if (quota.try_acquire(0)) overload[0]++;
            for (int w = 1; w < 8; ++w)
                if (quota.try_acquire(w)) overload[w]++;
            if (round % 50 == 49) quota.rebalance();
        }
    }

    printf("[A4] 场景 A（基准）each worker dispatch: ");
    for (int i = 0; i < 8; ++i) printf("%u ", baseline[i]);
    printf("\n");
    printf("[A4] 场景 B（worker0 超限）each worker dispatch: ");
    for (int i = 0; i < 8; ++i) printf("%u ", overload[i]);
    printf("\n");

    int pass_count = 0;
    for (int w = 1; w < 8; ++w) {
        uint32_t thresh = baseline[w] * 9 / 10;
        bool ok = overload[w] >= thresh;
        printf("[A4] worker_%d dispatch=%u, threshold(90%% of baseline)=%u, pass=%s\n",
               w, overload[w], thresh, ok ? "YES" : "NO");
        if (ok) pass_count++;
    }
    printf("[A4] non-overload workers pass: %d/7\n", pass_count);
    assert(pass_count == 7 && "A4 FAIL: at least one worker starved below 90% baseline");
    puts("[A4] PASS");
    return 0;
}
