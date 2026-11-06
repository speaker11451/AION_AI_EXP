#include "aion/m1/m1_core.hpp"
#include "../test_paths.hpp"
#include <cstdio>
#include <vector>
#include <random>
#include <chrono>
#include <algorithm>
using namespace aion::m1;
using namespace aion::m1::l0;
using namespace aion::m1::l3_5;

int main() {
    M1SchedulerCore core;
    core.wheel.global_burst.limit_per_tick = 10000;

    constexpr size_t N = 10000;
    std::vector<NodeProbe> nodes(N);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> tau_dist(1.0f, 30000000.0f);
    for (size_t i = 0; i < N; ++i) {
        nodes[i].id = i;
        nodes[i].tau = LogTau::from_us(tau_dist(rng));
    }
    TimePoint now{0};
    for (auto& n : nodes) core.schedule(&n, now);

    constexpr uint64_t TOTAL_TICKS = 1000000;
    constexpr uint64_t STEP_US = 1000;

    std::vector<uint64_t> dispatch_times_us;
    dispatch_times_us.reserve(TOTAL_TICKS);
    uint64_t total_disp = 0;

    for (uint64_t tick = 1; tick <= TOTAL_TICKS; ++tick) {
        now.us += STEP_US;
        auto t0 = std::chrono::high_resolution_clock::now();
        int count = 0;
        NodeProbe* n = core.dispatch(now, tick);
        while (n) {
            count++;
            core.schedule(n, now);
            n = core.dispatch(now, tick);
        }
        auto t1 = std::chrono::high_resolution_clock::now();
        uint64_t dur_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(t1 - t0).count();
        dispatch_times_us.push_back(dur_ns / 1000);
        total_disp += count;
    }

    std::sort(dispatch_times_us.begin(), dispatch_times_us.end());
    size_t p50 = dispatch_times_us.size() * 50 / 100;
    size_t p99 = dispatch_times_us.size() * 99 / 100;
    size_t p999 = dispatch_times_us.size() * 999 / 1000;

    FILE* f = open_artifact("M1/gate/D0/G2_1M_tick_latency.csv", "w");
    if (!f) { fprintf(stderr, "latency fopen failed\n"); return 1; }
    fprintf(f, "percentile,latency_us\n");
    fprintf(f, "p50,%llu\n",   (unsigned long long)dispatch_times_us[p50]);
    fprintf(f, "p99,%llu\n",   (unsigned long long)dispatch_times_us[p99]);
    fprintf(f, "p99.9,%llu\n", (unsigned long long)dispatch_times_us[p999]);
    fprintf(f, "max,%llu\n",   (unsigned long long)dispatch_times_us.back());
    fclose(f);

    printf("=== G2 10^6 tick 结果 ===\n");
    printf("total_dispatched = %llu\n", (unsigned long long)total_disp);
    printf("p50   = %llu us\n",   (unsigned long long)dispatch_times_us[p50]);
    printf("p99   = %llu us\n",   (unsigned long long)dispatch_times_us[p99]);
    printf("p99.9 = %llu us\n",   (unsigned long long)dispatch_times_us[p999]);
    printf("max   = %llu us\n",   (unsigned long long)dispatch_times_us.back());

    if (dispatch_times_us[p99] > 80) {
        printf("WARN: p99 > 80us, A5 全扫方案仅作临时, M2 必须换 O(1)\n");
    } else {
        printf("OK: p99 <= 80us, 临时方案在 M1 场景可接受\n");
    }
    return 0;
}
