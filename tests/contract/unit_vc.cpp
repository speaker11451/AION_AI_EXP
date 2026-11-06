#include "aion/m1/l3/vector_clock.hpp"
#include <cassert>
#include <cstdio>
#include <map>
using namespace aion::m1::l3;

int main() {
    // No-truncation case: MAX=64, only 8 nodes -> exact match with oracle
    VectorClockT<64> vc;
    std::map<uint32_t, uint64_t> oracle;
    for (uint32_t n = 0; n < 8; ++n) {
        for (uint64_t e = 1; e <= 5; ++e) {
            vc.merge(n, e);
            oracle[n] = e;
        }
    }
    for (auto& [node, ep] : oracle) {
        assert(vc.compare(node, ep) == 0);
    }

    // Truncation with MAX=4
    VectorClockT<4> tc;
    tc.merge(1, 10); tc.merge(2, 20); tc.merge(3, 30); tc.merge(4, 40);
    // Now n_slots == 4, adding node 5 with epoch 5 evicts smallest (1,10)
    tc.merge(5, 5);
    // node 5 is live now (evicted (1,10)), but node 1 is in summary_max=10.
    // compare(1, 10) -> 1 (covered by summary)
    // compare(1, 11) -> 2 (unknown, >summary_max)
    assert(tc.compare(1, 10) == 1);
    assert(tc.compare(1, 11) == 2);
    // compare live nodes is still exact
    assert(tc.compare(2, 20) == 0);
    assert(tc.compare(2, 21) == -1);
    // node 5 was merged; but it replaced (1,10).  compare(5,5)==0
    assert(tc.compare(5, 5) == 0);

    std::puts("unit_vc OK");
    return 0;
}
