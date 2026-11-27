---
name: danche-dingxiang
description: >-
  全国大学生智能汽车竞赛「单车定向」完整内置知识库。含 N1 车模、TC264、IMU 惯导、横滚平衡串级、
  Pure Pursuit/Stanley/杜宾斯、FOC、调参与翻车排查、算法伪代码。用户提到单车定向、N1、八字、
  绕锥、平衡车、惯导、TC264、Stanley、航位推算时必须使用；直接读本目录 refs/ 与 corpus/，
  不要要求用户打开外部知识库文件夹。
---

# 单车定向 Skill（全文已安装）

## 内置路径（本 skill 目录内）

| 文件 | 用途 |
|------|------|
| `refs/LEARNING_SUMMARY.md` | 学习摘要 |
| `refs/INDEX.md` | 检索索引 D1–D9 / B1–B12 |
| `refs/FILE_STRUCTURE.md` | 工程文件与依赖 |
| `refs/ALGORITHMS.md` | 算法拆解 + 可写代码模板 |
| `refs/COMPARE_AND_OPT.md` | 对比、优化、国赛风险、调参顺序 |
| `refs/DEEP_NOTES.md` / `MINE_SIGNALS.md` | 长文与关键词 |
| `corpus/*.txt` | 47 份参赛报告正文 |

回答前优先 Read 上述 refs；需要证据再搜 corpus。

## 赛道摘要

- 组别：单车定向；车模 **N1**；室外直线/八字/草地坡道绕锥；禁负压辅助平衡  
- 硬件：多 **TC264** + FOC(CYT2BL3) + BDS3620 + BDS300 + IMU + 磁编  
- 软件：`姿态EKF → INS → 寻迹 →（平衡优先）舵机+UART电机`  
- 主锚报告思路：中北破晓（`Attitude.c`/`imu.c`/`balance.c`/`car_control.c`）

## 控制优先级（必须遵守）

1. 横滚平衡环不倒  
2. 速度环  
3. 寻迹/航向  
4. 显示与日志必须降频，禁止拖垮控制周期  

## 调参默认顺序

静态 IMU → 架空舵机/电机/UART → 低速只开平衡 → 采样轨迹写 Flash → IMU_Go → Pure Pursuit → 分科目速度表  

## 用户点名展开

`展开 B6` / `写平衡环` / `对比 Stanley` / `排查低速倒` → 读 `refs/ALGORITHMS.md` 或 `COMPARE_AND_OPT.md` 对应节作答。
