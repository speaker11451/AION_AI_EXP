#include "aion/m1/l0/l0.hpp"
using namespace aion::m1::l0;
int main() {
    SurpriseNats sn{10.0f};
    IntensiveRate ir{5.0f};
    // 不同 Tag 不允许相加
    auto x = sn + ir;
    return 0;
}
