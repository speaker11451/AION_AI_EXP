#include "aion/m1/m1_core.hpp"
#include <cstdio>
#include <vector>
#include <random>
using namespace aion::m1;
using namespace aion::m1::l0;
using namespace aion::m1::l3_5;

int main() {
    {
        M1SchedulerCore core;
        NodeProbe n; n.tau = LogTau::from_us(1000.0f);
        core.schedule(&n, TimePoint{0});
        if (core.dispatch(TimePoint{1500}, 1) != &n) return 1;
        puts("[smoke] OK");
    }
    M1SchedulerCore core;
    core.wheel.global_burst.limit_per_tick = 10;
    constexpr size_t N = 10000;
    std::vector<NodeProbe> nodes(N);
    std::mt19937 rng(42);
    std::uniform_real_distribution<float> tau_long(16000000.0f, 30000000.0f);

    FILE* f_stats = fopen("../../artifacts/M1/stress/g2_stats.csv", "w");
    FILE* f_timeline = fopen("../../artifacts/M1/stress/g2_timeline.csv", "w");
    fprintf(f_stats, "tick_us,total_dispatched,burst_throttled\n");
    fprintf(f_timeline, "tick_us,node_id,tau_us,expiry_us\n");

    for (size_t i = 0; i < N; ++i) {
        nodes[i].id = i;
        nodes[i].tau = LogTau::from_us(i < 5000 ? 10.0f : tau_long(rng));
    }
    TimePoint now{0};
    for (auto& n : nodes) core.schedule(&n, now);

    uint64_t total_disp = 0;
    for (uint64_t tick = 1; tick <= 1000; ++tick) {
        now.us += 1000;
        NodeProbe* n = core.dispatch(now, tick);
        while (n) {
            total_disp++;
            if (total_disp < 2000)
                fprintf(f_timeline, "%llu,%u,%.1f,%.1f\n", (unsigned long long)now.us, n->id, n->tau.to_us(), (double)n->wheel_node.expiry_us);
            core.schedule(n, now);
            n = core.dispatch(now, tick);
        }
        if (tick % 10 == 0)
            fprintf(f_stats, "%llu,%llu,%llu\n", (unsigned long long)now.us, (unsigned long long)total_disp, (unsigned long long)core.wheel.global_burst.total_throttled);
    }
    fclose(f_stats); fclose(f_timeline);
    printf("G2 stress finished: total_dispatched=%llu, burst_throttled=%llu\n", (unsigned long long)total_disp, (unsigned long long)core.wheel.global_burst.total_throttled);
    return 0;
}
