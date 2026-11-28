# 智慧医疗（创意）· 知识摘要

## 软件链路

```text
检测/VLM → 任务（送药/巡检）→ 导航 P06 → 底盘/机械臂 C05 → S03
```

## 模块习惯

- ROS2 包：perception / task / nav / chassis  
- 见 `CREATIVE_ROS2_SKELETON.md`  
- 对照：`LX-HMKK/SmartCar_V2026` 等 RECOMMENDED

## 风险

VLM 延迟、导航卡住、臂与底盘互锁；仿真通再上实车。
