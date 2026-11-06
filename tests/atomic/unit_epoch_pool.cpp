#include "aion/m1/l3/epoch_pool.hpp"
#include <cassert>
#include <cstdio>
using namespace aion::m1::l3;

int main() {
    EpochPool pool;
    const uint32_t N = 3;

    float feat[4] = {1.0f, 2.0f, 3.0f, 4.0f};
    for (uint64_t ep = 1; ep <= 5; ++ep) {
        feat[0] = float(ep);
        pool.append_history(N, ep, feat, 4);
        pool.rotate(N);
    }

    // find_at_or_before(ep=3) should return snap with epoch == 3
    const CompactSnapshot* s = pool.find_at_or_before(N, 3);
    assert(s != nullptr);
    assert(s->epoch == 3);

    // find_at_or_before(ep=10) -> newest <=10, i.e. epoch=5
    s = pool.find_at_or_before(N, 10);
    assert(s != nullptr);
    assert(s->epoch == 5);

    // find_at_or_before(ep=0) -> nothing
    s = pool.find_at_or_before(N, 0);
    assert(s == nullptr);

    // acquire/release refcount
    EpochPool::Slot* slot = pool.acquire(N);
    assert(slot != nullptr);
    assert(slot->refcount.load() == 1);
    EpochPool::Slot* slot2 = pool.acquire(N);
    assert(slot2 == slot);
    assert(slot->refcount.load() == 2);
    pool.release(slot);
    assert(slot->refcount.load() == 1);
    pool.release(slot2);
    assert(slot->refcount.load() == 0);

    // history depth: after 20 appends only latest 16 remain valid
    EpochPool pool2;
    for (uint64_t ep = 1; ep <= 20; ++ep) {
        float f = float(ep);
        pool2.append_history(N, ep, &f, 1);
        pool2.rotate(N);
    }
    const CompactSnapshot* newest = pool2.find_at_or_before(N, 100);
    assert(newest != nullptr && newest->epoch == 20);
    // epoch 5 should be evicted (only 6..20 live, 15 entries but depth 16 so also 5 possible)
    // Actually depth=16, we wrote 20 entries -> last 16 = 5..20 are still present.
    const CompactSnapshot* old5 = pool2.find_at_or_before(N, 5);
    assert(old5 != nullptr && old5->epoch == 5);
    // epoch 4 evicted
    const CompactSnapshot* old4 = pool2.find_at_or_before(N, 4);
    assert(old4 == nullptr);

    std::puts("unit_epoch_pool OK");
    return 0;
}
