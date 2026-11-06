#include "aion/m2/replay_log.hpp"
#include <cstdio>
#include <cstring>
#include <cassert>
using namespace aion::m2;

int main() {
    ReplayLog log;
    assert(!log.is_enabled());
    assert(!log.push(1, 2, (const uint8_t*)"hi", 2));
    assert(log.size() == 0);

    log.enable(true);
    const char* msg1 = "hello";
    const char* msg2 = "world";
    log.push(1, 100, (const uint8_t*)msg1, std::strlen(msg1));
    log.push(2, 200, (const uint8_t*)msg2, std::strlen(msg2));
    assert(log.size() == 2);

    auto* a0 = log.at(0);
    auto* a1 = log.at(1);
    assert(a0 && a1);
    assert(a0->epoch == 1 && a0->eproc == 100);
    assert(a1->epoch == 2 && a1->eproc == 200);
    assert(a0->payload_hash_lo != a1->payload_hash_lo);

    ReplayLog log2; log2.enable(true);
    log2.push(1, 100, (const uint8_t*)msg1, std::strlen(msg1));
    assert(log2.at(0)->payload_hash_lo == a0->payload_hash_lo);
    assert(log2.at(0)->payload_hash_hi == a0->payload_hash_hi);

    puts("test_replay_log OK");
    return 0;
}
