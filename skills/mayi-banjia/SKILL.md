---
name: mayi-banjia
description: >-
  全国大学生智能汽车竞赛「蚂蚁搬家」完整技能：本地开源工程 + 报告知识 + 从零搭建软件清单。
  用于搭建/改写智能车软件（感知、状态机、控制环、执行器安全、日志）。
  触发：蚂蚁搬家、搬运、OpenART、RT1021、机械臂、推杆。用户提到蚂蚁搬家或要写/改该组代码时必须使用；
  按 SCAFFOLD→RECOMMENDED→ANALYSIS→refs 顺序执行。
---

# 蚂蚁搬家 · 可搭建软件 Skill

## 调用顺序（强制）
0. 提问要算法/结构 → `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`


1. `code_opensource/SCAFFOLD.md` — 从零搭建检查清单  
2. `code_opensource/RECOMMENDED.md` — 选对照/脚手架工程  
3. `code_opensource/ANALYSIS.md` — 软件结构与模块划分  
4. `refs/FULL_SUMMARY.md` + `refs/ALGORITHMS.md` + `corpus/`（若有）  
5. 需要模板/算法 ID → `../smartcar-ml-optimize/refs/`  
6. 调试现象 → `../smart-car-dev/refs/debug-playbook.md`

本地已挂接 `refs/` + `corpus/`（报告知识）。

## 默认软件栈

主控+视觉(OpenART/摄像头) + 定位/路径 + 执行机构(推杆/夹爪) 任务状态机

## 推荐对照仓

- `21届_蚂蚁搬家__github__LanternCX__SmartCar2026-TransportCar`
- `21届_蚂蚁搬家__github__dongtianbu__2026SmartCarAntMoving`
- `21届_蚂蚁搬家__gitee__kouyafu__eleven_team`
- `21届_蚂蚁搬家__github__jirencai__ants_smartcar`
- `21届_蚂蚁搬家__github__qkkkb__Ant_home_21`

完整度索引：`code_opensource/INDEX.md`（共 28 仓，完整 11）

## 改代码 / 搭模块输出格式

```
【组别】蚂蚁搬家
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
