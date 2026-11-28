# 创意组 ROS2 通用骨架

公开仓偏少时，用本骨架从零搭「可开环走 + 一种感知」。  
适用：智慧医疗 / 工厂 / 救援 / 城市 / 农场（按任务改包名与话题）。

## 工作空间

```text
ros2_ws/
  src/
    bringup/                  # launch + params
      launch/robot.launch.py
      config/params.yaml
    perception_pkg/
      perception_pkg/detect_node.py
    task_pkg/
      task_pkg/mission_fsm.py
    nav_pkg/                  # 或依赖 nav2
      nav_pkg/simple_nav.py
    chassis_pkg/
      chassis_pkg/cmd_vel_bridge.py
  README.md
```

## 话题约定

| 话题 | 类型（示意） | 谁发 |
|------|--------------|------|
| `/camera/image_raw` | Image | 驱动/仿真 |
| `/perception/targets` | PoseArray / 自定义 | perception |
| `/mission/state` | String | task |
| `/goal_pose` | PoseStamped | task |
| `/cmd_vel` | Twist | nav |
| `/odom` | Odometry | chassis |

## 节点职责

1. **detect_node**：传感器 → targets（带 stamp；失败发空）  
2. **mission_fsm**：任务阶段；超时回 IDLE；急停订阅  
3. **simple_nav**：goal → cmd_vel（先 P 控制即可）  
4. **cmd_vel_bridge**：Twist → 串口/CAN；看门狗停转  

## 最小联调顺序

1. chassis：键盘/固定 Twist 开环走  
2. perception：录包回放出 targets  
3. fsm：人工 goal 跑通状态迁移  
4. 再接真实检测与站点逻辑  

## 可落盘样例

仓库根：`generated/creative_ros2_ws/`（perception / task / nav / chassis / bringup）。  
有 ROS2：`colcon build && ros2 launch bringup robot.launch.py`。

## 与赛道 Skill

各 `zhihui-*/code_opensource/ANALYSIS.md` 指向本文件；  
算法链仍用 `smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`。
