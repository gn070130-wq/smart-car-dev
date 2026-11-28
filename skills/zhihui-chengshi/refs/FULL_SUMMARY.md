# 智慧城市（创意）· 知识摘要

## 软件链路

```text
车道/障碍/灯 → 任务 S01（站点/接驳）→ 规划 P06 → 底盘 C05
```

## 模块习惯

- `lane_det` / `city_fsm` / `planner` / `control`  
- 优先「可开环走 + 一种感知」再叠城市规则  
- 骨架：`CREATIVE_ROS2_SKELETON.md`

## 风险

公开 Robotaxi 仓少；规则元素分阶段加，避免一次堆满。
