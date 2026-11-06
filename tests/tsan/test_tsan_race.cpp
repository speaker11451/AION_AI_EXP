#include <atomic>
#include <thread>
#include <cstdio>
#include <barrier>
#include "aion/m1/l3/seqlock.hpp"
using namespace aion::m1::l3;

struct Payload { uint64_t a; uint64_t b; };

int main() {
    Seqlock<Payload> sl;
    sl.write({0, 0});
    std::barrier sync_point(2);
    std::atomic<bool> done{false};

    std::thread writer([&] {
        for (uint64_t i = 1; i <= 200000; ++i) sl.write({i, i});
        done.store(true);
        sync_point.arrive_and_wait();
    });

    std::thread reader([&] {
        while (!done.load()) {
            Payload p{};
#ifdef AION_TSAN_TEST
            // 注入竞态：直接读 data_ 引用（绕过 read()/fence），
            // 写入点在 writer 线程，读取点在 reader 线程，TSan 应报 race
            const Payload& leaked = sl.raw_data();
            volatile uint64_t sink = leaked.a; (void)sink;
#endif
            sl.read(p);
        }
        sync_point.arrive_and_wait();
    });

    writer.join();
    reader.join();
    puts("tsan_test finished");
    return 0;
}
