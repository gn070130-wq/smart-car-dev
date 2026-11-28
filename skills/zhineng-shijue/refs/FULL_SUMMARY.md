# 智能视觉 · 报告级知识摘要

流程不变。主控常见 **RT1064**；虚实结合/HIL 常见。

## 软件链路

```text
俯视建图视觉 + 第一人称视觉
→ 栅格/A* / 推箱求解
→ 状态机 → 麦轮逆解 + PID
```

## 模块习惯

- `map_vision_*` / `fpv_vision_*` / `a_star_*` / `sokoban_*` / `omni_pid_*` / `fsm_*`  
- 板库：`../../smart-car-dev/board_libs/seekfree_RT1064_Library`  
- 循迹子问题可复用 `VISION_LINE_PIPELINE.md`

## 风险

透视误差、推箱无解/卡死、双视觉不同步、仿真到实车迁移。

对照：`code_opensource/RECOMMENDED.md`；算法：`ALGORITHMS.md`。
