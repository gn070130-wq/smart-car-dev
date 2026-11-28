# 卡丁快跑 · 结构分析

对照仓（优先）：`21届_卡丁快跑__github__Yusei-YK__Polaris-Kart-TC387`  
主控/板级：TC387（ADS）  
默认算法链：`V05/V06→C09 或 P04→CTE→C01 + S01`  
对外接口：`target_err / cte → steer_pid；gate → speed`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/
  app/main.c isr_tick.c
  perception/vision_or_encoder.*
  planning/cte_replay.* / element_fsm.*
  control/steer_pid.* speed_gate.*
  actuators/safety.*
  bsp/TC387
对照：generated/board_tc387_stub/
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `cte` | path, pose | cte, heading_err |
| `speed_gate` | cte, element | v_set |
| `steer_pid` | cte/err | steer |
| `voice_fsm` | cmd | mode |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

高速饱和、CTE 采样距、科目切换冲击；多层速度门控。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_卡丁快跑__github__Yusei-YK__Polaris-Kart-TC387`

```text
user/   cpu0_main.c … cpu3_main.c isr.* kart_multicore.*
code/
  Kart_Algo/      kart_vision vtrack preprocess pid calc
  Kart_App/       kart_control imu odom pedal person_link light
  Kart_Decision/  mission motion follow playback menu params
  Kart_Driver/    camera encoder steer_abs power horn flash
  Kart_Config/    board_pins.h kart_calib.h
  Kart_Debug/     uart bench hw_test …
libraries/  infineon + zf_*
docs/  tools/  tests/
```

映射：`Kart_Algo`→感知/CTE；`Kart_Decision`→S01/科目；`Kart_App`→控制与惯导；多核入口在 `user/`。
host 壳：`generated/board_tc387_stub/`。
