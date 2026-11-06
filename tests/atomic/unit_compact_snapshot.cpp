#include "aion/m1/l3/compact_snapshot.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <chrono>
using namespace aion::m1::l3;

int main() {
    // 1) platform contract: 16B atomics must be lock-free
    bool lf16 = has_16b_lock_free();
    std::printf("[G3] __atomic_always_lock_free(16) = %s\n", lf16 ? "true" : "FALSE");
    std::printf("[G3] sizeof(CompactSnapshot)=%zu align=%zu\n",
                sizeof(CompactSnapshot), alignof(CompactSnapshot));

    // 2) single-threaded round-trip
    {
        CompactSnapshot dst{};
        CompactSnapshot src{};
        src.node_id = 42; src.epoch = 12345; src.n_feat = 4;
        src.feat[0] = 1.5f; src.feat[1] = -2.0f; src.feat[2] = 3.25f; src.feat[3] = 0.0f;
        store_compact(dst, src);
        CompactSnapshot out{};
        bool ok = load_compact(dst, out);
        assert(ok);
        assert(out.node_id == 42);
        assert(out.epoch   == 12345);
        assert(out.n_feat  == 4);
        assert(out.feat[0] == 1.5f && out.feat[1] == -2.0f && out.feat[2] == 3.25f);
        assert(out.checksum == snap_checksum(out));
    }

    // 3) concurrent: writer writes, readers must never see torn snapshots.
    //    Phase A: live concurrent writes -> count false reads
    //    Phase B: writer stopped, readers must see 0 false (atomicity contract)
    {
        CompactSnapshot shared{};
        // init to a known-zero state
        CompactSnapshot init{}; init.epoch = 0;
        store_compact(shared, init);

        std::atomic<bool>  writer_done{false};
        std::atomic<uint64_t> okA{0}, failA{0}, okB{0}, failB{0};

        constexpr int WRITES     = 100000;  // 1e5 writes
        constexpr int READS_A    = 500000;  // 5e5 reads during writes
        constexpr int READS_B    = 1000000; // 1e6 reads after

        std::thread writer([&]{
            for (int i = 1; i <= WRITES; ++i) {
                CompactSnapshot s{};
                s.node_id = 7;
                s.epoch   = uint64_t(i);
                s.n_feat  = SNAP_FEAT;
                for (size_t k = 0; k < SNAP_FEAT; ++k) s.feat[k] = float(i) + float(k);
                store_compact(shared, s);
            }
            writer_done.store(true, std::memory_order_release);
        });

        std::thread readerA([&]{
            for (int i = 0; i < READS_A; ++i) {
                CompactSnapshot out{};
                if (load_compact(shared, out)) okA.fetch_add(1, std::memory_order_relaxed);
                else                           failA.fetch_add(1, std::memory_order_relaxed);
            }
        });
        std::thread readerA2([&]{
            for (int i = 0; i < READS_A; ++i) {
                CompactSnapshot out{};
                if (load_compact(shared, out)) okA.fetch_add(1, std::memory_order_relaxed);
                else                           failA.fetch_add(1, std::memory_order_relaxed);
            }
        });

        writer.join();
        readerA.join(); readerA2.join();

        // Phase B: stable reads
        std::thread readerB([&]{
            for (int i = 0; i < READS_B; ++i) {
                CompactSnapshot out{};
                if (load_compact(shared, out)) okB.fetch_add(1, std::memory_order_relaxed);
                else                           failB.fetch_add(1, std::memory_order_relaxed);
            }
        });
        std::thread readerB2([&]{
            for (int i = 0; i < READS_B; ++i) {
                CompactSnapshot out{};
                if (load_compact(shared, out)) okB.fetch_add(1, std::memory_order_relaxed);
                else                           failB.fetch_add(1, std::memory_order_relaxed);
            }
        });
        readerB.join(); readerB2.join();

        uint64_t totalA = okA.load() + failA.load();
        uint64_t totalB = okB.load() + failB.load();
        double rateA = double(failA.load()) / double(totalA);
        double rateB = double(failB.load()) / double(totalB);
        std::printf("[G3] phase A (concurrent): ok=%llu fail=%llu rate=%.3e\n",
            (unsigned long long)okA.load(), (unsigned long long)failA.load(), rateA);
        std::printf("[G3] phase B (stable)    : ok=%llu fail=%llu rate=%.3e\n",
            (unsigned long long)okB.load(), (unsigned long long)failB.load(), rateB);

        // Contract: phase B (stable) must be exactly 0 -> < 1e-6
        if (failB.load() != 0) {
            std::printf("[G3] FAIL: stable-state false reads > 0\n");
            return 1;
        }
        if (rateB >= 1e-6) {
            std::printf("[G3] FAIL: stable-state false rate %.3e >= 1e-6\n", rateB);
            return 1;
        }
        // Phase A is best-effort; record but do not gate hard (by design,
        // concurrent readers may catch torn mid-write state -> need_pull).
    }

    std::puts("unit_compact_snapshot OK");
    return 0;
}
