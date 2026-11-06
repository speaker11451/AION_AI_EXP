#pragma once
namespace aion::m1::l3_5 {
struct PullTokenBucket {
    static constexpr int MAX_TOKENS = 2;
    int tokens{MAX_TOKENS};
    bool try_acquire() noexcept { if (tokens > 0) { --tokens; return true; } return false; }
    void release() noexcept { if (tokens < MAX_TOKENS) ++tokens; }
    bool exhausted() const noexcept { return tokens == 0; }
};
} // namespace aion::m1::l3_5
