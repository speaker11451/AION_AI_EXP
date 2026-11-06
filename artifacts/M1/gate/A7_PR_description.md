# A7: ReplayLog 骨架 — PR 描述

## 内存预算
- 65536 条 × 32B = **2.00 MB / 节点**
- 测试子图 10 节点 = 20 MB；100 节点 = 200 MB

## 生产关闭
- 运行时 flag：`enabled_ = false`（非编译宏，避免 ABI 分裂）
- 启动日志：`[A7] ReplayLog DISABLED (production)`

## 哈希组合方式（独立双家族）
- `payload_hash_lo` = **FNV-1a 64**（offset basis 0xCBF29CE484222325，prime 0x100000001b3）
- `payload_hash_hi` = **SplitMix64** 混入（offset 0x9E3779B97F4A7C15 ^ len<<32，multipliers 0xBF58476D1CE4E5B9 / 0x94D049BB133111EB）
- **不做异或**，两路 64-bit 分别独立存储，拼接即为 128-bit
- 生日碰撞界：2^64，65536 条目下碰撞概率 < 1e-19

## 重放一致性验收
- 两次回放同一消息序列，逐条比对 `(epoch, eproc, payload_hash_lo, payload_hash_hi)` bit 级一致
- 测试桩：`tests/m2/test_replay_log.cpp`
