---
name: smartcar-ml-optimize
description: >-
  智能车算法推荐与模块灌入：V/P/C/S 算法总表、赛道默认链路、代码模板 T1–T8、
  YOLO/NCNN/量化部署。用户提到推荐算法、算法ID、按赛题写代码、代码结构模块、
  YOLO、分割、RKNN、NPU、完善代码、按已学算法写模块时必须使用；与 smart-car-dev 的
  FROM_PROBLEM_TO_CODE / QUESTION_TO_ARCHITECTURE 配合，负责算法选型与模板落码。
---

# 算法推荐 + 模板落码

## 与提问流程的分工

| 环节 | 谁负责 |
|------|--------|
| 听懂问题、可编译落码 | `smart-car-dev` → `COMPILE_READY_WORKFLOW.md` |
| 听懂问题、输出整套结构 | `smart-car-dev` → `QUESTION_TO_ARCHITECTURE.md` |
| 算法 ID / 默认链 / 模板代码 | **本 skill** `refs/` |
| YOLO/分割等**模型训练脚本** | **本 skill** `workflow.md` / `DL_FROM_CORPUS.md` |
| 赛道报告证据 | 各赛道 `refs/` + `corpus/` |

## 灌入库（必读）

| 文件 | 用途 |
|------|------|
| `refs/INGESTED_ALGORITHMS.md` | 算法总表 V/P/C/S |
| `refs/TRACK_ALGORITHM_MAP.md` | 赛道默认模块链 |
| `refs/CODE_TEMPLATES.md` | T1–T8 可粘贴模板 |
| `refs/DL_FROM_CORPUS.md` | DL/NPU 策略 |
| `refs/dl-playbook.md` | 检测分割量化速查 |
| `refs/workflow.md` | 学习→优化→部署 |

## 强制步骤（写/荐算法时）

1. 定组别 → `TRACK_ALGORITHM_MAP` 默认链  
2. 展开 `INGESTED_ALGORITHMS` 每个 ID（必选/可选/不建议）  
3. 需要结构时提醒走 `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`  
4. 落码用 `CODE_TEMPLATES`；DL 读异步推理与新鲜度  
5. 输出【算法表】+【模块】；有工程则【原→改→说明→测试】  

## 输出模板

```
【组别】…
【算法推荐】
| 优先级 | ID | 名称 | 理由 | 关键参数 | 风险 |
【默认链路】V..→P..→C..→S..
【建议目录/接口】（或指向 SOFTWARE_SKELETON）
【模板】T.. 改写为用户语言
【测试】…
```

## 约束

控制环非阻塞；推理异步；限幅；超时安全态；关键点可打日志。
