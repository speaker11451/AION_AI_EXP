#!/bin/sh
echo "===== [G5] Anti-pattern audit ====="
echo "1. Checking for banned headers in include/:"
if grep -rE "#include <(vector|map|unordered_map|set|string|chrono)>" include/ ; then
    echo "FAIL: Forbidden headers detected"
    exit 1
else
    echo "PASS: No banned headers."
fi

echo "2. Checking for forbidden allocations (malloc/new) in hot path headers:"
if grep -rE "\b(malloc|calloc|realloc|new)\b" include/aion/m1/l0 include/aion/m1/l1 include/aion/m1/l3_5 ; then
    echo "FAIL: Dynamic allocation detected in hot path"
    exit 1
else
    echo "PASS: No dynamic allocation in hot path."
fi

echo "3. Checking for glibc/chrono in time handling:"
if grep -rE "std::chrono" include/ ; then
    echo "FAIL: std::chrono detected"
    exit 1
else
    echo "PASS: No std::chrono."
fi

echo "AUDIT OK"
