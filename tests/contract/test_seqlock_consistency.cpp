#include "aion/m1/l3/seqlock.hpp"
#include <cstdio>
#include <atomic>
#include <thread>
using namespace aion::m1::l3;
// 24 字节 > 128 位，aarch64 无法单指令原子加载
struct Payload { uint64_t a; uint64_t b; uint64_t c; };

int main() {
    Seqlock<Payload> sl;
    sl.write({0, 0, 0});
    std::atomic<bool> stop{false};
    std::atomic<uint64_t> read_torn{0}, raw_torn{0}, read_ok{0}, raw_ok{0};

    std::thread writer([&]{
        for (uint64_t i = 1; i <= 1000000; ++i) sl.write({i, i, i});
        stop.store(true);
    });
    std::thread reader([&]{
        while (!stop.load()) {
            Payload p{};
            if (sl.read(p)) {
                if (p.a == p.b && p.b == p.c) read_ok.fetch_add(1);
                else                          read_torn.fetch_add(1);
            }
            // raw_data() 绕过 read() 的 fence，直接访问 data_
            const Payload& leaked = sl.raw_data();
            uint64_t a = leaked.a, b = leaked.b, c = leaked.c;
            if (a == b && b == c) raw_ok.fetch_add(1);
            else                  raw_torn.fetch_add(1);
        }
    });
    writer.join(); reader.join();

    printf("read()     ok=%llu torn=%llu\n",
        (unsigned long long)read_ok.load(), (unsigned long long)read_torn.load());
    printf("raw_data() ok=%llu torn=%llu\n",
        (unsigned long long)raw_ok.load(), (unsigned long long)raw_torn.load());

    if (read_torn.load() != 0) { fprintf(stderr, "FAIL: read() torn\n"); return 1; }
    if (raw_torn.load() == 0)  { fprintf(stderr, "FAIL: raw_data() 未观测到撕裂\n"); return 1; }
    puts("PASS");
    return 0;
}
