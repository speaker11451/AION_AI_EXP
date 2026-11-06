#!/bin/bash
# CI 失败一键诊断：拉日志 + 分类 + 提取关键信息
set +e

RUN_ID="${1:-$(gh run list --workflow=m1-tsan.yml --limit=1 --json databaseId --jq '.[0].databaseId')}"
LOG_DIR="${2:-artifacts/M1/gate/D0}"
mkdir -p "$LOG_DIR"

echo "===== 诊断 run: $RUN_ID ====="

# 1. 拉状态
gh run view "$RUN_ID" 2>&1 | tee "$LOG_DIR/run_status.log" | head -n 20

# 2. 拉失败日志
gh run view "$RUN_ID" --log-failed > "$LOG_DIR/failed.log" 2>&1
echo "失败日志: $LOG_DIR/failed.log ($(wc -l < "$LOG_DIR/failed.log") 行)"

# 3. 分类
echo ""
echo "===== 失败分类 ====="
if grep -q "ThreadSanitizer: data race" "$LOG_DIR/failed.log"; then
    echo "🔴 [真竞态] TSan 检出 data race："
    grep -B1 -A8 "ThreadSanitizer: data race" "$LOG_DIR/failed.log" | head -n 30
    echo ""
    echo "   → 如果是 Seqlock 内部：白名单没生效，查 tsan.supp 路径/语法"
    echo "   → 如果不是 Seqlock：代码有真实竞态，必须修代码"
elif grep -q "Unable to find executable" "$LOG_DIR/failed.log"; then
    echo "🟡 [路径问题] 找不到测试可执行文件："
    grep -A3 "Unable to find executable" "$LOG_DIR/failed.log" | head -n 10
elif grep -q "fopen failed\|No such file" "$LOG_DIR/failed.log"; then
    echo "🟡 [文件路径问题]"
    grep -A3 "No such file\|fopen" "$LOG_DIR/failed.log" | head -n 10
elif grep -q "error:" "$LOG_DIR/failed.log"; then
    echo "🟡 [编译错误]："
    grep "error:" "$LOG_DIR/failed.log" | head -n 10
else
    echo "⚠️ 未匹配已知模式，人工看 $LOG_DIR/failed.log"
fi

# 4. 检查白名单
echo ""
echo "===== 白名单检查 ====="
if [ -f tsan.supp ]; then
    echo "tsan.supp 存在，内容："
    cat tsan.supp
    echo ""
    echo "   → TSan 白名单语法是 suppression_type:pattern（如 race:function）"
    echo "   → 参考: https://github.com/google/sanitizers/wiki/ThreadSanitizerSuppressions"
else
    echo "❌ tsan.supp 不存在！"
fi

echo ""
echo "===== 诊断完成 ====="
