---
name: zouma-guanbei
description: >-
  全国大学生智能汽车竞赛「走马观碑」完整技能：本地开源工程 + 报告知识 + 从零搭建软件清单。
  用于搭建/改写智能车软件（感知、状态机、控制环、执行器安全、日志）。
  触发：走马观碑、龙芯、LS2K、石碑、碑文。用户提到走马观碑或要写/改该组代码时必须使用；
  按 SCAFFOLD→RECOMMENDED→ANALYSIS→refs 顺序执行。
---

# 走马观碑 · 可搭建软件 Skill

## 调用顺序（强制）
0. 提问要算法/结构 → `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`


1. `code_opensource/SCAFFOLD.md` — 从零搭建检查清单  
2. `code_opensource/RECOMMENDED.md` — 选对照/脚手架工程  
3. `code_opensource/ANALYSIS.md` — 软件结构与模块划分  
4. `refs/FULL_SUMMARY.md` + `refs/ALGORITHMS.md` — 报告级算法与赛场经验  

5. 需要模板/算法 ID → `../smartcar-ml-optimize/refs/`  
6. 调试现象 → `../smart-car-dev/refs/debug-playbook.md`  
7. 龙芯/LS2K 解析 → `../smart-car-dev/references/repos/`（如 `ls2k0300`、`loognson`）+ `../smart-car-dev/refs/LS2K_LINUX_BOARD.md`

本地报告知识在 `refs/`；多仓解析在总控 `smart-car-dev/references/`（本 skill 无独立 `corpus/` 目录）。

## 默认软件栈

龙芯/Linux + 相机 + OCR/检测 + 导航与碑点任务状态机

## 推荐对照仓

- `届次待核_走马观碑__gitee__amireux_fish__Loognson`
- `21届_走马观碑__gitee__wuwusama__SmartCar_99Pai_OpenSource`
- `21届_走马观碑__gitee__damon3964__smart2026`
- `21届_走马观碑__gitee__HElla12312__2026-smart-car-competition`
- `21届_走马观碑__github__starco2025__smartcar-zmgb`

完整度索引：`code_opensource/INDEX.md`（共 11 仓，完整 6）

## 改代码 / 搭模块输出格式

```
【组别】走马观碑
【任务】搭建 | 改模块 | 调参 | 修 bug
【对照仓】…
【算法/模块】感知 / 规划状态机 / 转向环 / 速度环 / 执行器 / 日志
【原→改→说明→测试】
【风险】算力、延迟、丢线、饱和、安全互锁
```

## 硬约束

- 控制环非阻塞；推理异步；图像要有新鲜度/超时保护  
- 速度/PWM/舵机必须限幅；上电默认安全态  
- 优先改 `user`/`code`/`app`/`perception`/`control`，少动厂商底层库  
- 无用户工程时：按 SCAFFOLD 给出可粘贴目录树 + 主循环骨架 + 模块接口  
