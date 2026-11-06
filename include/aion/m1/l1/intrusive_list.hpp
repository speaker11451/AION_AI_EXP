#pragma once
#include <cstddef>
namespace aion::m1::l1 {
struct IntrusiveNode {
    IntrusiveNode* next{nullptr};
    IntrusiveNode* prev{nullptr};
};

class IntrusiveList {
public:
    IntrusiveList() noexcept { head_.next = head_.prev = &head_; }
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    bool empty() const noexcept { return head_.next == &head_; }
    IntrusiveNode* front() const noexcept { return empty() ? nullptr : head_.next; }

    void push_back(IntrusiveNode* n) noexcept {
        if (n->next != nullptr || n->prev != nullptr) return;
        n->prev = head_.prev;
        n->next = &head_;
        head_.prev->next = n;
        head_.prev = n;
    }

    void remove(IntrusiveNode* n) noexcept {
        if (!n || n == &head_ || !n->next || !n->prev) return;
        n->prev->next = n->next;
        n->next->prev = n->prev;
        n->next = n->prev = nullptr;
    }

    IntrusiveNode* pop_front() noexcept {
        if (empty()) return nullptr;
        IntrusiveNode* n = head_.next;
        remove(n);
        return n;
    }
private:
    IntrusiveNode head_;
};
} // namespace aion::m1::l1
