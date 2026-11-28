---
name: fengkuang-dianlu
description: >-
  全国大学生智能汽车竞赛「疯狂电路」完整技能：本地开源工程 + 报告知识 + 从零搭建软件清单。
  用于搭建/改写智能车软件（感知、状态机、控制环、执行器安全、日志）。
  触发：疯狂电路、电磁循迹、闪电鼠、TC264、TC364、电感、负压。用户提到疯狂电路或要写/改该组代码时必须使用；
  按 SCAFFOLD→RECOMMENDED→ANALYSIS→refs 顺序执行。
---

# 疯狂电路 · 可搭建软件 Skill

## 调用顺序（强制）
0. 提问要算法/结构 → `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`


1. `code_opensource/SCAFFOLD.md` — 从零搭建检查清单  
2. `code_opensource/RECOMMENDED.md` — 选对照/脚手架工程  
3. `code_opensource/ANALYSIS.md` — 软件结构与模块划分  
4. `refs/` + `corpus/` — 报告级算法与赛场经验（若有）  
5. 需要模板/算法 ID → `../smartcar-ml-optimize/refs/`  
6. 调试现象 → `../smart-car-dev/refs/debug-playbook.md`

本地已挂接 `refs/` + `corpus/`（报告知识）。

## 默认软件栈

TC264/364 + 多电感检赛道 + 中线估计 + 转向PD/纯跟踪 + 速度PID + 元素状态机

## 推荐对照仓

- `21届_疯狂电路__gitee__sleepallnigeh__21jiezhinengchefengkuangdianlu`
- `21届_疯狂电路__github__D-zzZzz__-`
- `21届_疯狂电路__github__dedehorse__21th-smart-car-vofa`
- `21届_疯狂电路__gitee__ren-peiju__crazy-circuit_21th`
- `21届_疯狂电路__gitee__chunxun-seeking-rain__tc364`

完整度索引：`code_opensource/INDEX.md`（共 24 仓，完整 15）

## 改代码 / 搭模块输出格式

```
【组别】疯狂电路
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
