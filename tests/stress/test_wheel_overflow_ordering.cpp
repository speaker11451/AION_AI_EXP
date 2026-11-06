#include "aion/m1/l3_5/time_wheel.hpp"
#include <cstdio>
#include <cassert>
#include <random>
#include <vector>
using namespace aion::m1::l3_5;
using namespace aion::m1::l0;

int main() {
    TimeWheel tw;
    tw.global_burst.limit_per_tick = 1000;
    // 强制单桶塞入 512 个节点
    std::vector<WheelNode> nodes(512);
    std::mt19937 rng(1337);
    // 都在同一个 64us 跨度内，同一层级
    for (int i = 0; i < 512; ++i) {
        nodes[i].expiry_us = 1000 + (rng() % 64);
        tw.schedule(&nodes[i], TimePoint{1000}, LogTau::from_us(float(nodes[i].expiry_us - 1000)), &nodes[i]);
    }
    // 依次派发，必须严格按 expiry 递增
    uint64_t last = 0;
    int count = 0;
    while (true) {
        WheelNode* n = tw.dispatch_expired(TimePoint{2000}, 1);
        if (!n) break;
        assert(n->expiry_us >= last);
        last = n->expiry_us;
        count++;
    }
    assert(count == 512);
    puts("[A5] overflow ordering OK");
    return 0;
}
