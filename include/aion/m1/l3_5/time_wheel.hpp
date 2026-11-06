#pragma once
#include <array>
#include <cstdint>
#include <cassert>
#include "aion/m1/l0/l0.hpp"
#include "aion/m1/l3_5/burst_breaker.hpp"

namespace aion::m1::l3_5 {
using namespace aion::m1::l0;

struct WheelNode {
    WheelNode* next{nullptr};
    WheelNode* prev{nullptr};
    uint64_t expiry_us{0};
    int level{-1};
    uint32_t bucket{0};
    bool in_wheel{false};
    void* owner{nullptr};
};

struct TimeWheel {
    static constexpr int LVL_COUNT = 4;
    static constexpr int BUCKETS   = 64;
    static constexpr int SUB_BUCKETS = 8;
    static constexpr uint64_t SHIFT[LVL_COUNT] = {0, 6, 12, 18};

    WheelNode* heads[LVL_COUNT][BUCKETS][SUB_BUCKETS];
    uint64_t next_expiry[LVL_COUNT];
    BurstBreaker global_burst;

    TimeWheel() noexcept {
        for (int l = 0; l < LVL_COUNT; ++l) {
            for (int b = 0; b < BUCKETS; ++b)
                for (int s = 0; s < SUB_BUCKETS; ++s) heads[l][b][s] = nullptr;
            next_expiry[l] = UINT64_MAX;
        }
        global_burst.limit_per_tick = 100;
    }

    void insert_sorted(WheelNode*& head, WheelNode* n) noexcept {
        n->next = nullptr; n->prev = nullptr;
        if (!head) { head = n; return; }
        WheelNode* cur = head, *tail = nullptr;
        while (cur) {
            if (n->expiry_us < cur->expiry_us) {
                n->next = cur; n->prev = cur->prev;
                if (cur->prev) cur->prev->next = n; else head = n;
                cur->prev = n;
                return;
            }
            tail = cur; cur = cur->next;
        }
        n->prev = tail; n->next = nullptr; tail->next = n;
    }

    void remove_from_bucket(int lvl, int b, int s, WheelNode* n) noexcept {
        WheelNode* head = heads[lvl][b][s];
        if (head == n) {
            heads[lvl][b][s] = n->next;
            if (n->next) n->next->prev = nullptr;
        } else {
            if (n->prev) n->prev->next = n->next;
            if (n->next) n->next->prev = n->prev;
        }
        n->next = nullptr; n->prev = nullptr;
    }

    void schedule(WheelNode* n, TimePoint now, LogTau tau, void* owner) noexcept {
        if (n->in_wheel) return;
        n->owner = owner;
        uint64_t delta = uint64_t(tau.to_us());
        if (delta == 0) delta = 1;
        uint64_t expiry = now.us + delta;
        n->expiry_us = expiry;

        int lvl = 3;
        if      (delta <= (1ULL << 6))  lvl = 0;
        else if (delta <= (1ULL << 12)) lvl = 1;
        else if (delta <= (1ULL << 18)) lvl = 2;

        n->level = lvl;
        uint32_t b = uint32_t((expiry >> SHIFT[lvl]) & (BUCKETS - 1));
        n->bucket = b;
        uint32_t sub = (SHIFT[lvl] > 3) ? ((expiry >> (SHIFT[lvl] - 3)) & 7) : 0;
        n->in_wheel = true;
        insert_sorted(heads[lvl][b][sub], n);
        if (expiry < next_expiry[lvl]) next_expiry[lvl] = expiry;
    }

    // 修复：跨桶跨亚桶的全局最小 expiry 扫描
    WheelNode* dispatch_expired(TimePoint now, uint64_t tick) noexcept {
        for (int lvl = 0; lvl < LVL_COUNT; ++lvl) {
            if (now.us < next_expiry[lvl]) continue;

            WheelNode* best = nullptr;
            int best_b = -1, best_s = -1;
            for (int b = 0; b < BUCKETS; ++b) {
                for (int s = 0; s < SUB_BUCKETS; ++s) {
                    WheelNode* n = heads[lvl][b][s];
                    if (!n) continue;
                    if (n->expiry_us > now.us) continue;
                    if (!best || n->expiry_us < best->expiry_us) {
                        best = n; best_b = b; best_s = s;
                    }
                }
            }

            if (best) {
                if (!global_burst.admit(tick)) {
                    LogTau new_tau = LogTau::from_us((best->expiry_us > now.us ? best->expiry_us - now.us : 1) * 10.0f);
                    remove_from_bucket(lvl, best_b, best_s, best);
                    best->in_wheel = false;
                    schedule(best, now, new_tau, best->owner);
                    return nullptr;
                }
                remove_from_bucket(lvl, best_b, best_s, best);
                best->in_wheel = false;
                return best;
            }

            bool all_empty = true;
            for (int b = 0; b < BUCKETS; ++b) {
                for (int s = 0; s < SUB_BUCKETS; ++s) {
                    if (heads[lvl][b][s]) { all_empty = false; break; }
                }
                if (!all_empty) break;
            }
            if (all_empty) next_expiry[lvl] = UINT64_MAX;
        }
        return nullptr;
    }
};
} // namespace aion::m1::l3_5
