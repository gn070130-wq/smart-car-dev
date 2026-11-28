# 蚂蚁搬家 · 结构分析

对照仓（优先）：`21届_蚂蚁搬家__github__LanternCX__SmartCar2026-TransportCar`  
主控/板级：视规则（OpenART / RT1021 + 主控）  
默认算法链：`V05/V06→C05→C01→S01 + P06/P09 + S04`  
对外接口：`target_xy → vx,vy,w；push_cmd；collab_frame`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
TransportCar/（或同类）
  app/state_machine.*     # 取货/搬运/放置/回库
  vision/ 或 OpenART 子仓  # 目标框/类
  planning/path_xy.*      # 点列/避障
  control/omni_ik.*       # 全向/差速
  actuators/arm_or_pusher.*
  collab/uart_frame.*     # 主从协同
  safety/
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `vision_target` | frame | xy, cls, stamp |
| `task_fsm` | target, pose | phase, push_cmd |
| `omni_ik` | vx,vy,w | wheel_pwm[] |
| `collab` | phase | peer_frame / stop |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

视觉超时、协同丢包、推杆卡死；丢包停机。对照 Vision/Controller 子仓。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_蚂蚁搬家__github__LanternCX__SmartCar2026-TransportCar`

```text
src/
  hardware/   encoders imu motors uart_bus
  control/    kinematics pid_* wheel
  role/       state_machine task_sync transport_plan follow_runtime
  play/       sequence / return_garage / startup_move
  protocol/   frame codec topic transport
  config/     comm motion safety vision …
vision/  controller/   # 子仓/联调（README 链 SmartCar2026-Vision/Controller）
docs/  tests/  seekfree_demo/
```

映射：`role/state_machine`→S01；`control/kinematics`→C05；`protocol`→S04；视觉结果经 config/vision + UART。
