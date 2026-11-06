#include "aion/m1/l3/seqlock.hpp"
#include <cassert>
#include <cstdio>
#include <thread>
#include <atomic>
using namespace aion::m1::l3;

struct Payload { uint64_t a; uint64_t b; };

int main() {
    // single-threaded write/read
    Seqlock<Payload> sl;
    sl.write({1, 2});
    Payload p{};
    assert(sl.read(p));
    assert(p.a == 1 && p.b == 2);

    // concurrent: writer toggles (a,b) between (i,i) pairs; reader must never
    // see torn state, i.e. a != b
    Seqlock<Payload> sl2;
    std::atomic<bool> stop{false};
    std::atomic<uint64_t> ok_reads{0}, bad_reads{0};
    sl2.write({0, 0});
    std::thread w([&]{
        for (uint64_t i = 1; i <= 200000; ++i) sl2.write({i, i});
        stop.store(true);
    });
    std::thread r([&]{
        while (!stop.load()) {
            Payload q{};
            if (sl2.read(q)) {
                if (q.a == q.b) ok_reads.fetch_add(1, std::memory_order_relaxed);
                else            bad_reads.fetch_add(1, std::memory_order_relaxed);
            }
        }
    });
    w.join(); r.join();
    assert(bad_reads.load() == 0);

    std::printf("unit_seqlock OK (ok=%llu bad=%llu)\n",
        (unsigned long long)ok_reads.load(), (unsigned long long)bad_reads.load());
    return 0;
}
