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

    // 场景 C（红队）：强制把一个 worker 的 quota 削减到 1，验证断言会拦住
    uint32_t starving[8] = {0};
    {
        GlobalBurstQuota quota;
        // 直接把 worker 3 的 quota 改成 1，模拟 rebalance 错误削减
        quota.per_worker_quota[3].store(1);
        for (int round = 0; round < 1000; ++round) {
            for (int w = 0; w < 8; ++w)
                if (quota.try_acquire(w)) starving[w]++;
            if (round % 50 == 49) quota.rebalance();
        }
    }

    printf("========== 场景 A（基准）==========\n");
    for (int i = 0; i < 8; ++i) printf("  worker_%d = %u\n", i, baseline[i]);

    printf("\n========== 场景 B（worker 0 每轮尝试 10 次）==========\n");
    printf("  worker_0 = %u (自己配额截断，不占用他人)\n", overload[0]);
    for (int i = 1; i < 8; ++i) printf("  worker_%d = %u\n", i, overload[i]);

    printf("\n========== 场景 C（红队：worker 3 强制 quota=1）==========\n");
    for (int i = 0; i < 8; ++i) printf("  worker_%d = %u\n", i, starving[i]);

    printf("\n========== 断言 1：场景 B 其余 7 worker >= 90%% baseline ==========\n");
    int pass_count = 0;
    for (int w = 1; w < 8; ++w) {
        uint32_t thresh = baseline[w] * 9 / 10;
        bool ok = overload[w] >= thresh;
        printf("  worker_%d dispatch=%u, threshold=%u, pass=%s\n",
               w, overload[w], thresh, ok ? "YES" : "NO");
        if (ok) pass_count++;
    }
    printf("  → %d/7 pass\n", pass_count);
    assert(pass_count == 7 && "A4 FAIL: 场景 B 有 worker 被饿死");

    printf("\n========== 断言 2（红队）：场景 C worker 3 必须 < 90%% baseline ==========\n");
    uint32_t thresh3 = baseline[3] * 9 / 10;
    bool starved = starving[3] < thresh3;
    printf("  worker_3 dispatch=%u, threshold=%u, starved=%s\n",
           starving[3], thresh3, starved ? "YES (断言正确拦截)" : "NO (断言失效)");
    assert(starved && "A4 FAIL: 饥饿场景没被断言拦住，测试无区分度");

    puts("\n[A4] PASS — 正常隔离 + 红队饥饿均正确识别");
    return 0;
}
