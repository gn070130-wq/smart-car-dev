# 飞檐走壁 · 结构分析

对照仓（优先）：`届次待核_飞檐走壁__gitee__W1512__smart-car`  
主控/板级：STC32G12K / G144（电磁吸附）  
默认算法链：`P02→C03/C01→S01 + S05（吸附/过流/倾覆）`  
对外接口：`imu,enc,current → pitch/speed_cmd → motor；mode_fsm`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/
  app/main.c              # 模式 IDLE/CLIMB/FAULT
  perception/imu_sense.*  # 姿态/倾角
  planning/climb_fsm.*    # 上墙/爬行/过角/下墙
  control/attitude_pid.*  # 俯仰/速度串级
  actuators/mag_drive.*   # 电磁/吸盘电流
  actuators/safety.*      # 过流、掉吸、倾覆急停
  bsp/                    # PWM ADC UART
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `climb_fsm` | 姿态/吸附状态 | mode, target_pitch |
| `attitude_pid` | pitch_err, enc | motor_duty |
| `mag_drive` | mode, current_fb | mag_pwm |
| `safety` | current, tilt, timeout | safe_hold |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

掉吸、过流、过角失稳、电磁发热；上电默认电机0+电磁关。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/届次待核_飞檐走壁__gitee__W1512__smart-car`

```text
code/缩微电磁1.4 - 比赛/   # 优先对照比赛版
  Project/
    user/  main.c isr.*
    mdk/   xunji.* task.* pid.* motor.* adc.* aoi.* imu660rb.*
           fuya.* encoder.* menu.* …
  Libraries/  zf_*
code/缩微电磁1.1 … 1.4/   # 迭代版本
```

映射：`xunji`→循迹；`task`→S01；`aoi/adc`→电磁吸附相关；`imu660rb`→P02；`motor/pid`→C01/C03。
