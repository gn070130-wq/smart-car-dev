# 飞跃雷区 · 结构分析

对照仓（优先）：`21届_飞跃雷区__gitee__wuqiuOvO__leiqu`  
主控/板级：飞控 + 地面车（双机/系留视方案）  
默认算法链：`V01/V04→C03/C07→S03→C05→C01`  
对外接口：`Air(tx,ty,conf,stamp) → Car(vx,vy,w)`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
air/                  # 飞控固件或脚本
  detect_beacon.*       # 灯/雷区目标
  telem_downlink.*      # UART/数传
ground/
  app/follow_fsm.*
  control/omni_or_diff.*
  safety/link_watchdog.*
shared/
  frame_proto.h         # CRC + stamp
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `beacon_det` | image | tx,ty,conf,stamp |
| `link` | frame | fresh / stale |
| `follow` | tx,ty | vx,vy,w |
| `safety` | stale,ms | safe_hover_or_stop |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

链路延迟、目标丢失、双机不同步；新鲜度超时必须安全态。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

优先结构清晰仓：`…__github__wuqiu-c__21_smartCar_feiyueleiqu`

```text
cyt4_macnum_car/              # 地面麦克纳姆
cyt4_flight_and_main_camera/  # 飞控+主相机
cyt2_sub_camera/              # 副相机
docs/  communicate.md device_app.md
```

多核整车对照：`…__hecatefake__21st-smartcar-flying-minefield` 的 `core0_cyt4bb7`…`core4_cyt4bb7`。

映射：空中检测→下行帧 `docs/communicate`；车端跟随在 macnum `project/`。
