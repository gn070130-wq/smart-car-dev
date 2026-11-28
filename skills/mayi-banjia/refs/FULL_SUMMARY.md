# 蚂蚁搬家 · 报告级知识摘要

流程不变：SCAFFOLD → RECOMMENDED → ANALYSIS → refs。

## 赛题要点

- 搬运：识别目标 → 接近 → 推杆/机构推物 → 确认 → 返回  
- 底盘多为全向；可选主从无线协同  
- 主控视规则：STC/RT 等；对照 `code_opensource/RECOMMENDED.md`

## 软件链路

```text
视觉目标(色块/检测) → 视觉伺服误差
全向逆解 + 速度 PID → 底盘
推杆/机构子状态机 → 到位确认（灰度/边线/超时）
任务 FSM：搜目标→对位→推→回巢
可选：主从 UART 帧协议协同
```

## 风险

丢目标、推偏、越界、双车失步、反光误检、控制周期被视觉拖死。

## 对照

- 开源：`code_opensource/RECOMMENDED.md`  
- 模块：`../../smart-car-dev/refs/MODULES_BSP.md`  
- 算法表：`ALGORITHMS.md`
