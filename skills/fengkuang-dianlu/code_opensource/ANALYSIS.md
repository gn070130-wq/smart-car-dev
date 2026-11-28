# 疯狂电路 · 结构分析

对照仓（优先）：`21届_疯狂电路__gitee__sleepallnigeh__21jiezhinengchefengkuangdianlu`  
主控/板级：TC264 / TC364 + 负压涵道  
默认算法链：`V01→V02→V03→S01→C02→C01 + C10`  
对外接口：`err, element → Δ,v；ducted_duty`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/
  app/main.c isr_tick.c
  perception/adc_inductor.*   # 多路电感
  perception/midline.*        # 电磁中线
  planning/element_fsm.*      # 十字/环岛/断路
  control/steer_pd.* speed_pi.*
  actuators/ducted_fan.*      # 负压
  actuators/safety.*
  bsp/adc pwm encoder
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `midline` | inductor[] | err, confidence |
| `element_fsm` | err, pattern | element_id, v_gate |
| `steer_pd` | err | delta |
| `ducted_fan` | element, battery | fan_duty |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

断路丢线、负压过流、环岛误判；丢线超时→安全态。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_疯狂电路__gitee__sleepallnigeh__21jiezhinengchefengkuangdianlu`

```text
TC264_350/TC264_350/TC264/     # ADS 工程（另有 TC264_200）
  user/   cpu0_main.c cpu1_main.c isr*.c
  code/   image.* control.* pid.* Motor.* encoder.* imu.* display.* system.*
  libraries/  infineon + zf_*（勿改核心）
```

映射：`image/control/pid` → 感知与环；`Motor` → 执行；负压若在他仓/分支再挂 `ducted_*`。
改代码优先动 `code/`，入口在 `user/cpu0_main.c`。
