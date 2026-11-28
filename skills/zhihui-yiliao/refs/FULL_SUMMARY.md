# 智慧医疗（创意）· 知识摘要

保持现有 skill 流程：SCAFFOLD → RECOMMENDED → ANALYSIS → 本 refs。

## 要点

- 常见栈：**ROS2** + 底盘 + 相机/VLM + 导航/机械臂（按当年规则）  
- 任务：巡检/识别标识/点到点运送等；推理与底盘控制解耦  
- 对照仓：`code_opensource/RECOMMENDED.md`（如 SmartCar_V2026、medical-robot）

## 软件链路

```text
相机/VLM(V06) → 目标与语义
导航(P06) / 局部避障 → 速度指令
底盘(C05) + 任务 FSM(S01) → 站点完成
安全(S03)：急停、超时、掉线停车
```

## 搭建顺序

1. 底盘遥控/键盘可走  
2. 单话题感知（检测或二维码）  
3. 导航到固定点  
4. 任务状态机串站点  
5. 日志与参数服务器  

总控骨架：`../../smart-car-dev/refs/SOFTWARE_SKELETON.md`（可映射 ROS2 包名）。  
算法 ID：`ALGORITHMS.md`。
