#include "aion/m1/l3_5/wait_graph.hpp"
#include <cassert>
#include <cstdio>
#include <vector>
using namespace aion::m1::l3_5;
using namespace aion::m1::l0;

int main() {
    WaitGraph wg;
    TimePoint now{100};

    // 构造环形等待：1 等 2，2 等 3，3 等 1
    wg.add(1, 2, now);
    wg.add(2, 3, now);
    wg.add(3, 1, now);

    // 触发 remove_all_involving(2)：
    // 1 的出边是 2（删除）；2 的入边是 1（删除）；2 的出边是 3（删除）；3 的入边是 2（删除）。
    // 剩下 3 等 1 的边不应受到影响。
    wg.remove_all_involving(2);

    // 剩余边应该是 3 -> 1
    assert(wg.edges.size() == 1);
    assert(wg.edges[0].waiter == 3 && wg.edges[0].waited == 1);

    // 再次触发 3，清空所有
    wg.remove_all_involving(3);
    assert(wg.edges.empty());
    assert(wg.reverse_index[1].empty());

    puts("unit_wait_graph_deadlock OK");
    return 0;
}
