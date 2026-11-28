# 智慧农场（创意）· 知识摘要

## 软件链路

```text
垄行/作物感知 → farm_fsm S01 → 底盘 C05 → S03
（模型可选，异步部署）
```

## 模块习惯

- `row_sense` / `farm_fsm` / `control`  
- 模型脚本可对照 RECOMMENDED 的 models 仓  
- 骨架：`SOFTWARE_SKELETON` + `CREATIVE_ROS2_SKELETON`

## 风险

打滑与域偏移；先沿垄，再作业机构。
