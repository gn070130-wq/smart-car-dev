# 轮腿穿越 · 结构分析

对照仓（优先）：`21届_轮腿穿越__gitee__ASDFAASDFWE__balanced_wheel_leg_car`  
主控/板级：CYT4BB7 双核  
默认算法链：`P02→C04→C06→差速 + S02`  
对外接口：`leg_len,leg_ang → q_joint[]；balance → wheel_torque`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
cm7_0/  # 控制核：平衡/腿/电机
cm7_1/  # 视觉核：锥桶/模式，写共享快照
shared/vision_to_ctrl.h
user/balance_* leg_ik_*
见 CYT4BB7_DUAL_CORE.md；generated/board_cyt4bb7_stub/
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `imu_balance` | gyro,acc,enc | wheel_torque |
| `leg_ik` | len,ang | q_joint[] |
| `vision_mode` | cone_det | mode_flag (IPC) |
| `ipc` | snapshot+stamp | freshness |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

视觉抢平衡、奇异位、双核不同步；平衡优先于寻迹。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_轮腿穿越__gitee__ASDFAASDFWE__balanced_wheel_leg_car`

```text
Blalance_wheel_leg_Feis_v1.4/
  project/user/
    main_cm7_0.c  main_cm7_1.c
    cm7_0_isr.c   cm7_1_isr.c
  libraries/ …
```

映射：CM7_0→平衡/腿/电机；CM7_1→视觉/模式；对照 `CYT4BB7_DUAL_CORE.md` 与 `generated/board_cyt4bb7_stub/`。
