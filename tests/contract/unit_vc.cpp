#include "aion/m1/l3/vector_clock.hpp"
#include <cassert>
#include <cstdio>
#include <map>
using namespace aion::m1::l3;

int main() {
    VectorClockT<64> vc;
    std::map<uint32_t, uint64_t> oracle;
    for (uint32_t n = 0; n < 8; ++n)
        for (uint64_t e = 1; e <= 5; ++e) { vc.merge(n, e); oracle[n] = e; }
    for (auto& [node, ep] : oracle) assert(vc.compare(node, ep) == 0);

    // D1 裁决：截断后，from_epoch < summary_min 必须返回 2 (unknown)
    VectorClockT<4> tc;
    tc.merge(1, 10); tc.merge(2, 20); tc.merge(3, 30); tc.merge(4, 40);
    tc.merge(5, 5); // 驱逐了 (1, 10)，summary_min = 10
    assert(tc.compare(1, 10) == 2); // 10 < summary_min? no, ==. 返回 2
    assert(tc.compare(1, 11) == 2); // 11 > summary_min
    assert(tc.compare(1, 5) == 2);  // 5 < summary_min 必须返回 2
    assert(tc.compare(2, 20) == 0);
    assert(tc.compare(2, 21) == -1);
    assert(tc.compare(2, 19) == 1);
    assert(tc.compare(5, 5) == 0);

    // 具体用例：summary_min=100, from_epoch=50 -> 2
    VectorClockT<2> tiny;
    tiny.merge(100, 100);
    tiny.merge(200, 200);
    tiny.merge(300, 50); // 驱逐 (100,100)，summary_min=100
    assert(tiny.compare(100, 50) == 2); // 50 < summary_min(100) -> 2
    assert(tiny.compare(100, 100) == 2);
    assert(tiny.compare(100, 150) == 2);

    std::puts("unit_vc OK - D1 semantics enforced (from_epoch < summary_min -> 2)");
    return 0;
}
