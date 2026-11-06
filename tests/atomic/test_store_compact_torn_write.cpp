#include "aion/m1/l3/compact_snapshot.hpp"
#include <cassert>
#include <cstdio>
#include <cstring>
#include <thread>
#include <atomic>
#include <barrier>
using namespace aion::m1::l3;

int main() {
    CompactSnapshot shared{};
    CompactSnapshot init{}; init.node_id = 1; init.epoch = 1;
    store_compact(shared, init);

    std::barrier sync_point(2);
    std::atomic<bool> stage_ready{false};

    std::thread writer([&]{
        CompactSnapshot src{};
        src.node_id = 2; src.epoch = 2; src.n_feat = 4;
        for (int i = 0; i < 4; ++i) src.feat[i] = float(i + 10);
        
        CompactSnapshot tmp = src;
        tmp.seq = shared.seq + 1; tmp.checksum = 0;
        char* d = reinterpret_cast<char*>(&shared);
        const char* s = reinterpret_cast<const char*>(&tmp);
        // 手动写前 4 段
        for (int seg = 0; seg < 4; ++seg) {
            __int128 v; std::memcpy(&v, s + seg * 16, 16);
            __atomic_store(reinterpret_cast<__int128*>(d + seg * 16), &v, __ATOMIC_RELEASE);
        }
        // 此时前 4 段已发布，checksum 还没写（保持旧值）
        stage_ready.store(true);
        sync_point.arrive_and_wait(); // 等待读线程尝试读取
        // 继续写第 5 段
        tmp.checksum = snap_checksum(tmp);
        __int128 v; std::memcpy(&v, s + 64, 16);
        __atomic_store(reinterpret_cast<__int128*>(d + 64), &v, __ATOMIC_RELEASE);
    });

    while (!stage_ready.load()) std::this_thread::yield();
    // 此时写线程写完了前 4 段，但没写 checksum，读线程读取必须返回 false
    CompactSnapshot out{};
    bool ok = load_compact(shared, out);
    if (ok) { fprintf(stderr, "[A3] FAIL: torn write must return false!\n"); return 1; }
    puts("[A3] torn write correctly returned false");

    sync_point.arrive_and_wait(); // 让写线程写完
    writer.join();

    ok = load_compact(shared, out);
    if (!ok) { fprintf(stderr, "[A3] FAIL: final read must succeed\n"); return 1; }
    if (out.node_id != 2 || out.epoch != 2) { fprintf(stderr, "[A3] FAIL: data mismatch\n"); return 1; }
    puts("[A3] final read correctly succeeded");
    return 0;
}
