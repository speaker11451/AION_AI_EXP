#!/bin/bash
# 本地预检：推送前跑一遍，减少 CI 翻车
set +e
cd "$(dirname "$0")/.."

echo "===== 本地预检开始 ====="
FAIL=0

# 1. 全量构建
echo "[1/5] 全量构建..."
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release >/dev/null 2>&1
cmake --build build -j 2>&1 | grep -E "error|Error" && FAIL=1 || echo "  ✅ 构建通过"

# 2. 全量测试
echo "[2/5] 全量 ctest..."
(cd build && ctest --output-on-failure > /tmp/local_ctest.log 2>&1)
CTEST_RC=$?
if [ $CTEST_RC -eq 0 ]; then
    echo "  ✅ ctest 全绿"
else
    echo "  ❌ ctest 失败:"
    tail -n 15 /tmp/local_ctest.log
    FAIL=1
fi

# 3. 白名单语法检查
echo "[3/5] 白名单语法检查..."
if [ -f tsan.supp ]; then
    while IFS= read -r line; do
        [[ "$line" =~ ^#.*$ || -z "$line" ]] && continue
        if [[ "$line" =~ ^(race|thread|called_from_lib|mutex|signal): ]]; then
            echo "  ✅ 合法: $line"
        else
            echo "  ❌ 非法 suppression: $line"
            FAIL=1
        fi
    done < tsan.supp
else
    echo "  ⚠️ tsan.supp 不存在"
fi

# 4. git status 检查
echo "[4/5] 未提交文件检查..."
if [ -n "$(git status --porcelain)" ]; then
    echo "  ⚠️ 有未提交文件："
    git status --porcelain | head -n 5
fi

# 5. workflow 文件检查
echo "[5/5] workflow 文件检查..."
if [ -f .github/workflows/m1-tsan.yml ]; then
    echo "  ✅ workflow 存在"
    # 检查 YAML 基本语法（用 python 解析）
    python3 -c "import yaml; yaml.safe_load(open('.github/workflows/m1-tsan.yml'))" 2>/dev/null \
        && echo "  ✅ YAML 语法合法" || echo "  ❌ YAML 语法错误"
else
    echo "  ⚠️ workflow 不存在"
fi

echo ""
if [ $FAIL -eq 0 ]; then
    echo "===== 预检通过，可以推送 ====="
    exit 0
else
    echo "===== 预检失败，修复后再推 ====="
    exit 1
fi
