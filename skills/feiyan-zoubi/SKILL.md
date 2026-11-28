---
name: feiyan-zoubi
description: >-
  全国大学生智能汽车竞赛「飞檐走壁」完整技能：本地开源工程 + 报告知识 + 从零搭建软件清单。
  用于搭建/改写智能车软件（感知、状态机、控制环、执行器安全、日志）。
  触发：飞檐走壁、爬墙、电磁壁面、STC32、微型电磁。用户提到飞檐走壁或要写/改该组代码时必须使用；
  按 SCAFFOLD→RECOMMENDED→ANALYSIS→refs 顺序执行。
---

# 飞檐走壁 · 可搭建软件 Skill

## 调用顺序（强制）
0. 提问要算法/结构 → `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`


1. `code_opensource/SCAFFOLD.md` — 从零搭建检查清单  
2. `code_opensource/RECOMMENDED.md` — 选对照/脚手架工程  
3. `code_opensource/ANALYSIS.md` — 软件结构与模块划分  
4. `refs/FULL_SUMMARY.md` + `refs/ALGORITHMS.md` — 报告级算法与赛场经验  

5. 需要模板/算法 ID → `../smartcar-ml-optimize/refs/`  
6. 调试现象 → `../smart-car-dev/refs/debug-playbook.md`

本地已挂接 `refs/` + `corpus/`（报告知识）。

## 默认软件栈

MCU(STC32/同类) + 电磁/姿态 + 壁面控制状态机 + 速度/转向环

## 推荐对照仓

- `届次待核_飞檐走壁__gitee__W1512__smart-car`
- `21届_飞檐走壁__gitee__mtdj__smartcar-code-hub`
- `届次待核_飞檐走壁__gitee__null_mian__smart-car-defies-gravity`
- `21届_飞檐走壁__github__Jeans2__2026-smart-car-wall-climbing-electromagnetic`
- `21届_飞檐走壁__gitee__arknightssss__21st-smart-car-acrobatics-team`

完整度索引：`code_opensource/INDEX.md`（共 30 仓，完整 19）

## 改代码 / 搭模块输出格式

```
【组别】飞檐走壁
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
