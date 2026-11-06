#!/bin/bash
# 触发 CI → 监控 → 自动诊断 → 落盘
set +e
cd "$(dirname "$0")/.."
mkdir -p artifacts/M1/gate/D0

WORKFLOW="${1:-m1-tsan.yml}"

echo "===== 触发 workflow: $WORKFLOW ====="
gh workflow run "$WORKFLOW" --ref master
sleep 5

echo ""
echo "===== 等待运行启动 ====="
for i in $(seq 1 10); do
    STATUS=$(gh run list --workflow="$WORKFLOW" --limit=1 --json status --jq '.[0].status')
    if [ "$STATUS" = "in_progress" ] || [ "$STATUS" = "completed" ]; then
        break
    fi
    sleep 3
done

RUN_ID=$(gh run list --workflow="$WORKFLOW" --limit=1 --json databaseId --jq '.[0].databaseId')
echo "run id = $RUN_ID"

echo ""
echo "===== 监控中 ====="
gh run watch "$RUN_ID" || true

echo ""
echo "===== 自动诊断 ====="
./tools/ci_diag.sh "$RUN_ID"
