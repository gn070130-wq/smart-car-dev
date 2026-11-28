---
name: danche-dingxiang
description: >-
  全国大学生智能汽车竞赛「单车定向」完整技能：本地开源工程 + 报告知识 + 从零搭建软件清单。
  用于搭建/改写智能车软件（感知、状态机、控制环、执行器安全、日志）。
  触发：单车定向、自行车、N1、八字、绕锥、Stanley。用户提到单车定向或要写/改该组代码时必须使用；
  按 SCAFFOLD→RECOMMENDED→ANALYSIS→refs/FULL_SUMMARY 顺序执行。
---

# 单车定向 · 可搭建软件 Skill

## 调用顺序（强制）
0. 提问要算法/结构 → `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`


1. `code_opensource/SCAFFOLD.md` — 从零搭建检查清单  
2. `code_opensource/RECOMMENDED.md` — 选对照/脚手架工程  
3. `code_opensource/ANALYSIS.md` — 软件结构与模块划分  
4. `refs/FULL_SUMMARY.md` + `refs/ALGORITHMS.md` — 报告级算法与赛场经验  

5. 需要模板/算法 ID → `../smartcar-ml-optimize/refs/`  
6. 调试现象 → `../smart-car-dev/refs/debug-playbook.md`


## 知识与对照位置

- 报告级：`refs/FULL_SUMMARY.md`、`refs/ALGORITHMS.md`（有则优先）  
- 开源元数据：`code_opensource/{SCAFFOLD,RECOMMENDED,INDEX,ANALYSIS}.md`  
- 完整开源树：本机 Cursor 工作区对应 skill 的 `code_opensource/`（Git 默认只存元数据）  
- 可编译/上板：`../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`、`BOARD_TEMPLATES.md`  
- 仓库说明：`../README.md`（skills 目录总表）  
- 板级：TC264+FOC → `IMU_INS_PLAYBOOK.md` + `board_tc264_stub`


## 默认软件栈

姿态/转向 + Stanley/纯跟踪 + 锥桶/八字任务状态机

## 推荐对照仓

- `21届_单车定向__gitee__xceline566__motorcycle`
- `21届_单车定向__github__Z-car__ZJX_21_Bike`
- `21届_单车定向__github__YYDW-i__Intelligent-bike`
- `届次待核_单车定向__gitee__kunpeng666__smartcar`
- `21届_单车定向__github__Amy-hub-creator__21-`

完整度索引：`code_opensource/INDEX.md`（共 6 仓，完整 5）

## 改代码 / 搭模块输出格式

```
【组别】单车定向
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
