#pragma once
#include <cstdint>
#include "aion/m1/l0/time_point.hpp"
#include "aion/m1/l1/fixed_vec.hpp"
namespace aion::m1::l3_5 {
using namespace aion::m1::l0;
using namespace aion::m1::l1;

struct WaitGraph {
    static constexpr size_t MAX_EDGES = 256;
    struct Edge { uint32_t waiter, waited; TimePoint now; };
    FixedVec<Edge, MAX_EDGES> edges;
    FixedVec<uint32_t, MAX_EDGES> reverse_index[128];

    void add(uint32_t waiter, uint32_t waited, TimePoint now) noexcept {
        if (edges.full()) return;
        Edge e{waiter, waited, now};
        edges.push_back(e);
        if (waited < 128) reverse_index[waited].push_back(waiter);
    }

    void remove(uint32_t waiter) noexcept {
        size_t i = 0;
        while (i < edges.size()) {
            if (edges[i].waiter == waiter) {
                uint32_t w = edges[i].waited;
                if (w < 128) {
                    // 在反向索引中彻底删除该 waiter
                    size_t j = 0;
                    while (j < reverse_index[w].size()) {
                        if (reverse_index[w][j] == waiter) {
                            for (size_t k = j; k + 1 < reverse_index[w].size(); ++k)
                                reverse_index[w][k] = reverse_index[w][k + 1];
                            reverse_index[w].pop_back(); // 修正：不是 clear，而是精确 pop
                            break;
                        }
                        ++j;
                    }
                }
                // 在正向边中删除：将尾部元素移到当前位置，再 pop_back
                edges[i] = edges[edges.size() - 1];
                edges.pop_back();
                // 不要 i++，因为新移过来的元素也需要被检查
            } else {
                ++i;
            }
        }
    }

    void remove_all_involving(uint32_t node_id) noexcept {
        // 1. 清空出边（自己做 waiter 的边）
        remove(node_id);
        // 2. 清空入边（自己作为 waited 的反向索引）
        if (node_id < 128) {
            while (reverse_index[node_id].size() > 0) {
                uint32_t w = reverse_index[node_id][0];
                remove(w); // 安全：remove 内部会精确减少反向索引
            }
        }
    }
};
} // namespace aion::m1::l3_5
