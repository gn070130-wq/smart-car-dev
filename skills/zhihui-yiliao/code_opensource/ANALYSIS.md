# 智慧医疗 · 结构分析

对照仓（优先）：`21届_智慧医疗__github__LX-HMKK__SmartCar_V2026`  
主控/板级：RDK / ROS2 常见  
默认算法链：`V06/VLM→P06→C05→S01→S03`  
对外接口：`detect → navigate → manipulate/chassis`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
ros2_ws/src/
  perception_pkg/     # 检测/VLM
  nav_pkg/            # Nav2 或自研
  task_pkg/           # 送药/巡检状态机
  chassis_pkg/        # 底盘驱动
  bringup/
见 CREATIVE_ROS2_SKELETON.md
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `detect` | image | target_pose |
| `task_fsm` | mission | nav_goal / arm_cmd |
| `nav` | goal | cmd_vel |
| `chassis` | cmd_vel | odom |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

VLM 延迟、导航卡住、机械臂互锁；仿真先通再实车。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_智慧医疗__github__LX-HMKK__SmartCar_V2026`

```text
src/
  smartcar_bringup/  smartcar_task/  smartcar_nav2/
  smartcar_vision/   smartcar_safety/ smartcar_sim/
  smartcar_interfaces/ smartcar_common/ origincar/ …
scripts/  competition_launch.py local_sim.sh nav_*.sh
config/  docs/  tests/
```

映射：ROS2 包≈`CREATIVE_ROS2_SKELETON`；任务在 `smartcar_task`，导航 `smartcar_nav2`。
