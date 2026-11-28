# PVC 赛道摄像头循迹代码包

依据 Skill：`VISION_LINE_PIPELINE` / `ELEMENT_FSM_LIBRARY` / `SOFTWARE_SKELETON`。

## 赛题理解（假设）

- **场地**：室内 PVC（反光、接缝常见）
- **任务**：循迹 + 直道/弯道 + 十字/环岛（可选）
- **传感**：灰度相机（默认 188×120，可改宏）+ 编码器 + 可选陀螺航向
- **执行**：差速两轮或舵机+后驱（本包输出 `pwm_l/pwm_r` 与归一化 `err`）

## 算法映射

| 任务 | 算法ID | 文件 |
|------|--------|------|
| 二值/抗反光阈值 | V01 | `track_pvc.*` |
| 搜边+中线 | V02/V03 | `track_pvc.*` |
| 环岛/十字 | S01 | `element_fsm.*` |
| 转向差速 | C02/C01 | `control_steer.*` |
| 主循环 | — | `app_track_main.c` |

## 目录

```text
pvc_line_track/
  track_pvc.h / track_pvc.c
  element_fsm.h / element_fsm.c
  control_steer.h / control_steer.c
  app_track_main.c
  README.md
```

## 本地编译（PC 验证）

```bat
cd generated\pvc_line_track
build_host.bat
```

或：

```bat
gcc -std=c11 -DPVC_TRACK_HOST_DEMO=1 -I. track_pvc.c element_fsm.c control_steer.c app_track_main.c hal_stub.c -o pvc_track_host.exe -lm
pvc_track_host.exe
```

## 接入你的工程

1. **不要**链 `hal_stub.c`；实现 `hal.h` 中接口（相机/时间/电机/航向）  
2. 若线色相反：编译加 `-DPVC_DARK_LINE=0`（白线）或保持默认黑线  
3. 先低速直道标定 `Kp/Kd`，再开环岛  
4. 上板工程只编 `pvc_track_core` 源文件 + 你的 HAL + 自己的 `main`（不要定义 `PVC_TRACK_HOST_DEMO`）

## 调参顺序

1. 静止看 `th`、左右边是否贴线  
2. 直道 `err≈0`，压左边 `err` 符号与打舵方向一致  
3. 弯道加大 `far_w`  
4. 环岛角 `RING_IN_ANGLE` / `RING_OUT_ANGLE`  
