#include "aion/m1/l3/spsc_queue.hpp"
#include "aion/m1/l3/mpsc_queue.hpp"
#include <cassert>
#include <cstdio>
#include <thread>
#include <atomic>
using namespace aion::m1::l3;

int main() {
    // SPSC single-threaded push/pop semantics
    SPSCQueue<int, 8> q;
    for (int i = 0; i < 8; ++i) assert(q.push(i));
    assert(!q.push(8));
    int v = -1;
    for (int i = 0; i < 8; ++i) { assert(q.pop(v)); assert(v == i); }
    assert(!q.pop(v));

    // SPSC two-thread: 100000 items
    {
        SPSCQueue<uint64_t, 1024> q2;
        std::atomic<uint64_t> produced{0}, consumed{0};
        std::thread prod([&]{
            for (uint64_t i = 0; i < 100000; ++i) {
                while (!q2.push(i)) {}
                produced.fetch_add(1, std::memory_order_relaxed);
            }
        });
        std::thread cons([&]{
            uint64_t expected = 0;
            while (expected < 100000) {
                uint64_t x;
                if (q2.pop(x)) { assert(x == expected); ++expected; consumed.fetch_add(1, std::memory_order_relaxed); }
            }
        });
        prod.join(); cons.join();
        assert(produced.load() == 100000);
        assert(consumed.load() == 100000);
    }

    // MPSC semantics: 4 producers x 5000 items, single consumer sees all
    {
        MPSCQueue<uint64_t, 1024> mq;
        std::atomic<int> done{0};
        std::thread cons([&]{
            uint64_t got = 0;
            while (got < 20000) {
                uint64_t x;
                if (mq.pop(x)) ++got;
            }
            done.store(1);
        });
        std::thread ps[4];
        for (int p = 0; p < 4; ++p) {
            ps[p] = std::thread([&, p]{
                for (uint64_t i = 0; i < 5000; ++i) {
                    while (!mq.push(uint64_t(p) * 100000u + i)) {}
                }
            });
        }
        for (auto& t : ps) t.join();
        cons.join();
        assert(done.load() == 1);
    }

    std::puts("unit_spsc_mpsc OK");
    return 0;
}
