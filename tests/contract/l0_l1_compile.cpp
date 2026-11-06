#include "aion/m1/l0/l0.hpp"
#include "aion/m1/l1/l1.hpp"
#include <cassert>
#include <cmath>
#include <cstdio>
using namespace aion::m1;

int main() {
    using namespace aion::m1::l0;
    using namespace aion::m1::l1;

    // L0
    LogTau t = LogTau::from_us(1000.0f);
    assert(std::isfinite(t.to_us()) && t.to_us() > 0);
    assert(LogTau::from_us(0.0f).to_us()  >= LogTau::MIN_US - 1e-3f);
    assert(LogTau::from_us(1e30f).to_us() <= LogTau::MAX_US + 1.0f);

    SurpriseNats sn{50.0f};
    sn += 5.0f;
    assert(sn.v >= SurpriseNats::MIN && sn.v <= SurpriseNats::MAX);

    auto ds = DampedSurprise::from_low_confidence(1.0f);
    assert(ds.confidence == 0.0f);
    auto dh = DampedSurprise::from_high_confidence(1.0f);
    assert(dh.confidence == 1.0f);

    SlewRateLimiter srl(0.5f, 0.0f);
    assert(std::fabs(srl.step(10.0f) - 0.5f) < 1e-6f);
    assert(std::fabs(srl.step(-10.0f) - 0.0f) < 1e-6f);

    SplitMix64 rng(0xDEADBEEFULL);
    uint64_t a = rng.next();
    uint64_t b = rng.next();
    assert(a != b);
    float f = rng.next_float();
    assert(f >= 0.0f && f < 1.0f);

    // L1
    FixedVec<int, 4> fv;
    for (int i = 0; i < 4; ++i) assert(fv.push_back(i));
    assert(!fv.push_back(99));
    assert(fv.size() == 4 && fv[0] == 0 && fv[3] == 3);

    RingBuffer<int, 4> rb;
    assert(rb.push(1) && rb.push(2) && rb.push(3) && rb.push(4));
    assert(!rb.push(5));
    int out = 0;
    assert(rb.pop(out) && out == 1);

    IntrusiveList il;
    IntrusiveNode n1{}, n2{};
    il.push_back(&n1);
    il.push_back(&n2);
    assert(il.front() == &n1);
    il.remove(&n1);
    assert(il.front() == &n2);
    assert(il.pop_front() == &n2);
    assert(il.empty());

    std::puts("contract_l0_l1 OK");
    return 0;
}
