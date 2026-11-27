---
name: yanguo-liuhen
description: >-
  全国大学生智能汽车竞赛「雁过留痕」完整内置知识库。含 STC32、MT9V034、边线/状态机、
  PID/模糊/纯跟踪、差速、激光打靶、专科+本科报告语料。用户提到雁过留痕、激光打靶、
  循迹 PID、环岛十字、STC32、神眼/总钻风时必须使用；直接读本目录 refs/、corpus/、
  corpus_benke/，不要要求用户打开外部文件夹。
---

# 雁过留痕 Skill（全文已安装）

## 内置路径（本 skill 目录内）

| 路径 | 用途 |
|------|------|
| `refs/FULL_SUMMARY.md` | 任务清单完整汇总（架构/算法/对比/评估） |
| `refs/LEARNING_SUMMARY.md` | 学习摘要 |
| `refs/INDEX.md` | 检索索引 A1–A12 / Y1–Y11 |
| `refs/COMPARE_AND_GAPS.md` | 对比与优化 |
| `refs/reference-architecture.md` | 软件/硬件架构 |
| `refs/reference-pid-control.md` | PID/纯跟踪/差速 |
| `refs/reference-vision-elements.md` | 视觉与元素状态机 |
| `refs/reference-laser-task.md` | 激光打靶安全逻辑 |
| `refs/corpus-index.md` | 专科精读优先级 |
| `refs/anchor_湖文孔明九队.md` | 主锚完整报告 |
| `refs/team_cards/` | 队卡 |
| `refs/DEEP_READ_*.md` | 深扫总表 |
| `corpus/` | 专科 9 队正文 |
| `corpus_benke/` | 本科报告正文 |
| `scripts/` | 提取/挖掘脚本 |

## 赛道摘要

- 组别：雁过留痕；室内 PVC；循迹 + **紫外激光留痕**  
- 硬件：多 **STC32G144K** + MT9V034/总钻风 + 双编码器 + IMU + 多路激光 + 常配涵道  
- 流水线：帧完成 → 阈值/边线/中线 → 元素状态机 → 转向(PD/模糊/纯跟踪) → 增量速度 PID+差速 → 激光脉冲  

## 默认选型

| 环 | 默认 |
|----|------|
| 转向 | 偏差 PD + 二次 P ± IMU 阻尼；或纯跟踪+舵机闭环 |
| 速度 | 增量式 PI/PID + 差速 |
| 激光 | 默认全关、单路互锁、ms 脉冲 |
| 元素 | 跨帧状态机 + 里程/帧计数退出 |

## 调试顺序

相机/阈值 → 边线 → 舵机中位 → 编码器极性 → 低速 PD → 元素表速 → 激光安全 → 再提速  

## 用户点名展开

读对应 `refs/reference-*.md` 或 `refs/INDEX.md` 编号；需要原文搜 `corpus/` / `corpus_benke/`。
