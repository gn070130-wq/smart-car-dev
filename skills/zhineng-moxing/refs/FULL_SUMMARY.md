# 智能模型 · 报告级知识摘要

流程不变。算力侧常见 **RK3588 NPU**；车控侧常 **TC264** 等经 UART。

## 软件链路

```text
摄像头 → RK3588(YOLO/分割/OCR, RKNN)
→ 偏差/目标帧 UART → 下位机 PID/电机舵机
```

## 本机对照仓

1. `X-SmartCar-AI-Model-Multitask` — 多任务  
2. `PaddleYOLO-RKNN` — 检测→RKNN  
3. `USTB_21stModel_MapEditor` — 地图工具  

部署要点见总控：`../../smart-car-dev/refs/RKNN_DEPLOY.md`（不改现有 COMPILE 流程，仅增文档）。

## 风险

推理超时掉帧、量化漏检、串口阻塞控制环、高速失稳。

算法卡：`ALGORITHMS.md`。
