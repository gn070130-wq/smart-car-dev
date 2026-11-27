---
name: feiyue-leiqu
description: >-
  全国大学生智能汽车竞赛「飞跃雷区」完整内置知识库。含系留飞行器+地面车、CYT4BB7/TRAVEO、
  信标连通域视觉、飞控串级 PID、高度 ADRC/EKF、麦轮/Y全向轧灯、有线 UART/RS-485。
  用户提到飞跃雷区、系留、轧灯、信标灯、麦轮轧灭、光流 ToF、空地协同时必须使用；
  直接读本目录 refs/ 与 corpus/，不要要求用户打开外部知识库文件夹。
---

# 飞跃雷区 Skill（全文已安装）

## 内置路径（本 skill 目录内）

| 路径 | 用途 |
|------|------|
| `refs/FULL_SUMMARY.md` | 架构/算法/对比/风险完整汇总 |
| `refs/LEARNING_SUMMARY.md` | 学习摘要 |
| `refs/INDEX.md` | 检索索引 F1–F10 / A1–A10 |
| `refs/ALGORITHMS.md` | 算法卡 |
| `refs/FILE_STRUCTURE.md` | 逻辑工程结构 |
| `refs/COMPARE_AND_RISKS.md` | 方案对比与竞赛风险 |
| `refs/DEEP_NOTES.md` | 深读笔记 |
| `refs/MINE_SIGNALS.md` | 语料信号总表 |
| `refs/INGEST_LOG.md` | 接收解析日志 |
| `corpus/` | 45 份报告正文 |

## 赛道摘要

- 组别：飞跃雷区；室内信标灯分批点亮  
- 系统：**悬停飞行器**（感知）+ **地面车**（轧灭）+ **≤1.5 m 系留**（供电+有线数据；任务禁无线）  
- 主控常见 **CYT4BB7/TRAVEO** 多核；电调 AI8051/STC32；视觉 MT9V03X 单/多目  
- 流水线：视觉连通域 → 飞控串级/高度水平 → 系留帧下发 → 车运动学+轮速环 → 轧灯  

## 默认选型

| 环 | 默认 |
|----|------|
| 信标 | 阈值+连通域质心+面积打分 |
| 姿态 | 串级 PID（角→角速度） |
| 高度 | ToF + PID/EKF；扰动大用 ADRC |
| 水平 | 像素误差→平面速度（随高度标定） |
| 车 | 麦轮/Y全向逆解 + 轮速 PID |
| 通信 | UART/RS-485 + CRC + 新鲜度超时 |

## 调试顺序

曝光/阈值 → 单灯质心稳定 → 飞控悬停 → 系留通信 CRC → 车开环跟点 → 轮速环 → 联调轧灯 → 多灯/批次 → 系留扰动与急停  

## 开发模式

可补代码/重构/修 bug；给可替换片段 + 改动说明 + 测试。优先非阻塞、限幅、过期保护。

## 用户点名展开

读 `refs/INDEX.md` 编号或 `refs/ALGORITHMS.md`；原文搜 `corpus/`。
