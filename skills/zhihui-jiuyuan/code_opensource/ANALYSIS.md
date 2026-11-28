# 智慧救援 · 结构分析

对照仓（优先）：`21届_智慧救援__github__ting5619__2026_smartcar_YLR8`  
主控/板级：多传感器底盘（ROS2/嵌入式均可）  
默认算法链：`感知目标 → S01 → C01/C05 → S03`  
对外接口：`victim/zone → nav_goal → chassis_cmd`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
ros2_ws/src/
  sense_pkg/      # 热成像/视觉/气体等（按题）
  task_pkg/       # 搜救任务机
  nav_pkg/
  chassis_pkg/
见 CREATIVE_ROS2_SKELETON.md
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `sense` | sensors | target, conf |
| `rescue_fsm` | target | phase, goal |
| `nav` | goal | cmd_vel |
| `safety` | tilt, link | abort |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

误报目标、复杂地形；先定点导航再叠搜救逻辑。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_智慧救援__github__ting5619__2026_smartcar_YLR8`

```text
src/
  launch_real/ user_navigation/ multi_goals_navigation/
  planning/ localization_mapping/ amcl_localization/ ekf_localization/
  ros_astra_camera/ ydlidar_ros_driver/ hipnuc_imu/
  r8c_move_base/ r8c_gmapping/ robot_package/ …
tools/  PLAN.md
```

映射：感知驱动→定位→`user_navigation`/`multi_goals`→底盘；对齐 `CREATIVE_ROS2_SKELETON`。
