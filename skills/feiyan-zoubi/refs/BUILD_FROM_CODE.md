# 飞檐走壁 · 由开源工程提炼的搭建要点

本组报告 KB 未单独挂接；以下来自本地推荐仓结构与通用骨架。

## 典型栈

- MCU：STC32 等 + Seekfree 库常见  
- 传感：电磁/姿态（IMU）/轮速  
- 控制：壁面吸附或电磁约束下的速度/姿态稳定 + 任务状态机  

## 搭建顺序

1. 复用推荐仓的 `Libraries`/`Project`/`user` 分层  
2. 先跑通电机/电磁驱动与安全停  
3. 再做姿态/电流保护与轨迹段状态机  
4. 通用目录参照 `../../smart-car-dev/refs/SOFTWARE_SKELETON.md`

## 改代码原则

优先 `user`/`Project` 业务文件；Seekfree 库只读引用。
