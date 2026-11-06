#include "aion/m2/replay_log.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>
using namespace aion::m2;

int main() {
    // 生产模式：关闭
    ReplayLog log;
    assert(!log.is_enabled());
    assert(!log.push(1, 2, (const uint8_t*)"hi", 2));
    assert(log.size() == 0);

    // 测试模式：开启
    log.enable(true);
    const char* msgs[] = {"alpha","beta","gamma","delta","epsilon"};
    for (int i = 0; i < 5; ++i)
        log.push(i+1, 100*(i+1), (const uint8_t*)msgs[i], std::strlen(msgs[i]));
    assert(log.size() == 5);

    // 重放两次 bit 级一致性
    ReplayLog log2; log2.enable(true);
    for (int i = 0; i < 5; ++i)
        log2.push(i+1, 100*(i+1), (const uint8_t*)msgs[i], std::strlen(msgs[i]));

    for (size_t i = 0; i < 5; ++i) {
        const auto* a = log.at(i);
        const auto* b = log2.at(i);
        assert(a && b);
        assert(a->epoch == b->epoch);
        assert(a->eproc == b->eproc);
        assert(a->payload_hash_lo == b->payload_hash_lo);
        assert(a->payload_hash_hi == b->payload_hash_hi);
    }
    puts("[A7] 重放 bit 级一致: PASS");

    // 独立性说明（散点检查）：不同 payload 的 (lo, hi) 都不同
    for (int i = 0; i < 5; ++i) {
        for (int j = i+1; j < 5; ++j) {
            auto* a = log.at(i);
            auto* b = log.at(j);
            assert(a->payload_hash_lo != b->payload_hash_lo);
            assert(a->payload_hash_hi != b->payload_hash_hi);
        }
    }
    puts("[A7] 双 hash 独立性散点检查: PASS");
    return 0;
}
