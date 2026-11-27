# 轮腿穿越 · 逻辑工程结构

| 模块 | 核 | 节拍 |
|------|----|------|
| imu/balance/wheel | 控制核 | 1 ms |
| leg_ik/joint | 控制核 | 1–5 ms |
| vision/element | 视觉核 | 按帧 |
| ipc | 双核 | 共享内存 |
