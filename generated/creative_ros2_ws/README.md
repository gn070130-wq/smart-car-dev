# 创意组最小 ROS2 工作空间（样例）

对照 `smart-car-dev/refs/CREATIVE_ROS2_SKELETON.md`。  
本目录为**可阅读/可改的包骨架**；在已装 ROS2 的机器上 `colcon build`。

## 包

| 包 | 作用 |
|----|------|
| `perception_pkg` | 假目标发布（targets） |
| `task_pkg` | 任务状态机 → goal |
| `nav_pkg` | goal → cmd_vel（P 控制示意） |
| `chassis_pkg` | cmd_vel 看门狗 + 打印桥 |
| `bringup` | 一键 launch |

## 本机（有 ROS2 时）

```bash
cd generated/creative_ros2_ws
rosdep install --from-paths src -y --ignore-src   # 可选
colcon build
source install/setup.bash
ros2 launch bringup robot.launch.py
```

无 ROS2 时：直接读各包 `*.py`，按赛道改话题与 FSM；医疗对照 `zhihui-yiliao` ★ 仓 `smartcar_*` 包名。
