# 智慧工厂（创意）· 知识摘要

公开开源少。先循迹开环，再叠锥桶/红绿灯元素。

## 软件链路

```text
线误差 V01/V05 → factory_fsm S01 → 转向/速度 C01/C02
→ 交通灯/锥桶门控 → S03 安全
```

## 模块习惯

- `line_*` / `traffic_*` / `factory_fsm_*` / `steer_speed_*`  
- 骨架：`SOFTWARE_SKELETON.md` + `CREATIVE_ROS2_SKELETON.md`  
- 对照：`code_opensource/RECOMMENDED.md`（仓薄时以 ANALYSIS 为准）

## 风险

元素误检、环岛丢线、仓源码过少导致照抄失败——按 ANALYSIS 自建模块。
