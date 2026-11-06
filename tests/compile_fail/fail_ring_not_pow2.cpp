#include "aion/m1/l1/l1.hpp"
using namespace aion::m1::l1;
int main() {
    RingBuffer<int, 10> rb; // 10 不是 2 的幂，必须 static_assert 失败
    return 0;
}
