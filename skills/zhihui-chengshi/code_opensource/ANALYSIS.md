# 智慧城市 · 结构分析

对照仓（优先）：`21届_智慧城市__gitee__chuxiao-dyf__2026_-robotaxi`  
主控/板级：Robotaxi 类（ROS2 / 百度套件常见）  
默认算法链：`感知(车道/障碍/灯) → P06 → C05 → S01`  
对外接口：`lane/obstacle/light → nav → chassis`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
ros2_ws/src/
  perception_pkg/
  planning_pkg/
  control_pkg/
  task_pkg/         # 站点/接驳
  bringup/
见 CREATIVE_ROS2_SKELETON.md
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `lane_det` | image | cte, light |
| `city_fsm` | mission, light | goal |
| `planner` | goal | path |
| `control` | path | cmd_vel |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

公开开源少；先「可开环走 + 一种感知」再叠城市规则。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`…__chuxiao-dyf__2026_-robotaxi/icar_autopilot_2026th_最终版`

```text
CMakeLists.txt  include/  src/  res/  README.md
```

C++ 自动驾驶样例；缺 ROS 包时用 `generated/creative_ros2_ws/` 补 bringup/task/nav 壳。
