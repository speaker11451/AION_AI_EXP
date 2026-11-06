#include "aion/m1/l3_5/global_burst_quota.hpp"
#include <cstdio>
#include <cstdlib>

#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "[A4] FAIL: %s\n", msg); return 1; } \
} while (0)

using namespace aion::m1::l3_5;

int main() {
    // 场景 A：基准
    uint32_t baseline[8] = {0};
    {
        GlobalBurstQuota quota;
        for (int round = 0; round < 1000; ++round) {
            for (int w = 0; w < 8; ++w)
                if (quota.try_acquire(w)) baseline[w]++;
            if (round % 50 == 49) quota.rebalance();
        }
    }

    // 场景 B：worker 0 每轮尝试 10 次，其余 7 worker 每轮 1 次
    uint32_t overload[8] = {0};
    uint32_t worker0_attempts = 0, worker0_success = 0;
    {
        GlobalBurstQuota quota;
        for (int round = 0; round < 1000; ++round) {
            for (int k = 0; k < 10; ++k) {
                worker0_attempts++;
                if (quota.try_acquire(0)) { overload[0]++; worker0_success++; }
            }
            for (int w = 1; w < 8; ++w)
                if (quota.try_acquire(w)) overload[w]++;
            if (round % 50 == 49) quota.rebalance();
        }
    }

    // 场景 C（红队）：worker 3 每轮强制 quota=1，rebalance 时只 reset used 不加 quota
    uint32_t starving[8] = {0};
    {
        GlobalBurstQuota quota;
        for (int round = 0; round < 1000; ++round) {
            quota.per_worker_quota[3].store(1);  // 每轮强制压到 1
            for (int w = 0; w < 8; ++w)
                if (quota.try_acquire(w)) starving[w]++;
            if (round % 50 == 49) {
                for (uint32_t i = 0; i < 8; ++i) quota.used[i].store(0);
                // 故意不调用 rebalance，防止 worker_3 的 quota 被恢复
            }
        }
    }

    printf("========== 场景 A（基准）==========\n");
    for (int i = 0; i < 8; ++i) printf("  worker_%d = %u\n", i, baseline[i]);

    printf("\n========== 场景 B（worker 0 每轮尝试 10 次）==========\n");
    printf("  worker_0 attempts=%u success=%u (quota 截断: 被拒 %u 次)\n",
           worker0_attempts, worker0_success, worker0_attempts - worker0_success);
    for (int i = 1; i < 8; ++i) printf("  worker_%d = %u\n", i, overload[i]);

    printf("\n========== 场景 C（红队：worker 3 每轮 quota 强制=1，rebalance 不恢复）==========\n");
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
    CHECK(pass_count == 7, "场景 B 有 worker 被饿死");

    printf("\n========== 断言 2（红队）：场景 C worker 3 必须 < 90%% baseline ==========\n");
    uint32_t thresh3 = baseline[3] * 9 / 10;
    bool starved = starving[3] < thresh3;
    printf("  worker_3 dispatch=%u, threshold=%u, starved=%s\n",
           starving[3], thresh3, starved ? "YES (断言正确拦截)" : "NO (断言失效)");
    CHECK(starved, "红队场景 C 未检出饥饿，断言无区分度");

    puts("\n[A4] PASS — 场景 B 无挤压 + 场景 C 饥饿被拦截");
    return 0;
}
