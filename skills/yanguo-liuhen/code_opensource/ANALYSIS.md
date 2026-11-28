# 雁过留痕 · 结构分析

对照仓（优先）：`21届_雁过留痕__github__Retard-cpu__21Car`  
主控/板级：STC32G144 + 神眼/总钻风 + 激光  
默认算法链：`V01→V02→V03→S01→C02→C01 + 激光脉冲安全`  
对外接口：`err, element_id, v_set → pwm_l,pwm_r, laser_en`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/
  app/main.c isr_tick.c
  perception/camera_line.*  # 边线/中线
  planning/element_fsm.*    # 圆环/闸门/打靶
  control/steer_pd.* speed.*
  actuators/laser_pulse.*   # 互锁
  actuators/safety.*
  bsp/
深度报告：本 skill refs/（DEEP_READ_* / team_cards）
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `track_line` | bin_img | err, confidence |
| `element_fsm` | err, roi | element, laser_req |
| `steer_pd` | err | delta |
| `laser_pulse` | laser_req, armed | laser_en |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

激光误触发、丢线、曝光；激光默认关，需 armed+元素确认。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_雁过留痕__github__Retard-cpu__21Car`

```text
project/
  user/   main.c isr.*
  code/   searchline.* preprocessing.* forecast.*
          island.* cross.* barrier.* zebra.* slope.*
          aim.* laser.* servo.* motor.* pid.* imu.* encoder.*
  mdk/    工程文件
libraries/  zf_*（只读）
```

映射：`searchline/preprocessing`→V01–V03；元素 `island/cross/…`→S01；`aim/laser`→打靶互锁；`pid/motor/servo`→C02/C01。
