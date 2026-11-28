# 单车定向 · 结构分析

对照仓（优先）：`21届_单车定向__gitee__xceline566__motorcycle`  
主控/板级：TC264 + FOC 无刷  
默认算法链：`P02→P03→P05→C04→C01 + C08`  
对外接口：`imu,enc → roll_cmd,yaw_rate,speed；path → steer_ff`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/
  perception/imu_ins.*
  planning/cone_fsm.* / eight_fsm.*
  control/balance_cascade.* stanley.*
  actuators/foc_uart.*
  actuators/safety.*
见 IMU_INS_PLAYBOOK.md
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `imu_ins` | imu,enc | roll,yaw,spd |
| `balance` | roll_err | torque_cmd |
| `stanley` | cte, heading | steer_ff |
| `foc_uart` | torque/spd | motor |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

平衡与路径抢权——平衡优先；滑移、FOC 通信超时。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_单车定向__gitee__xceline566__motorcycle`

```text
1.Hardware/   TC264 / CYT2BL3 原理
2.Firmware/   hex
3.Software/
  user/   cpu0_main.c cpu1_main.c isr.*
  code/   imu.* kalman.* motor.* image.* gnss.* analog.* menu.* …
  libraries/  zf_* + infineon
6.Seekfree/   TC264_Library / CYT2BL3 无刷工程
5.Docs/  4.CAD/  7.Tools/
```

映射：`imu/kalman`→P02/P03；`motor`→C08/C04；任务/锥桶逻辑多在 `code/`+`cpu0_main` 状态分支。
