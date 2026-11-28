# 智慧救援（创意）· 知识摘要

## 软件链路

```text
多传感感知 → rescue_fsm S01 → 导航 → 底盘 C01/C05 → S03
```

## 模块习惯

- `sense_*` / `rescue_fsm_*` / `nav_*` / `chassis_*`  
- ROS2：`CREATIVE_ROS2_SKELETON.md`  
- 对照：`ting5619/2026_smartcar_YLR8`（RECOMMENDED）

## 风险

误报目标、地形与倾覆；先定点导航再叠搜救。
