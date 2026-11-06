#pragma once
#include "aion/m1/l0/l0.hpp"
#include "aion/m1/l1/l1.hpp"
#include "aion/m1/l3_5/l3_5.hpp"

namespace aion::m1 {
using namespace aion::m1::l0;
using namespace aion::m1::l1;
using namespace aion::m1::l3_5;

struct NodeProbe {
    uint32_t id{0};
    LogTau tau;
    LogicalEpoch epoch;
    WheelNode wheel_node;
    TickBarrier barrier;
    FreezeCounter freeze;
    PullTokenBucket pull_tokens;
    uint64_t dispatch_count{0};
};

class M1SchedulerCore {
public:
    TimeWheel wheel;
    WaitGraph wait_graph;

    void schedule(NodeProbe* n, TimePoint now) noexcept {
        wheel.schedule(&n->wheel_node, now, n->tau, n);
    }
    NodeProbe* dispatch(TimePoint now, uint64_t tick) noexcept {
        WheelNode* wn = wheel.dispatch_expired(now, tick);
        if (!wn) return nullptr;
        return reinterpret_cast<NodeProbe*>(wn->owner);
    }
};
} // namespace aion::m1
