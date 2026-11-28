---
name: smart-car-dev
description: >-
  智能车软件总控：按用户要求调取 Skill 知识，经大模型分析生成可本地编译的代码；
  含摄像头循迹、元素状态机、电磁中线、Stanley/PP、TOF/编码器/红外/IMU。
  用户提到智能车、写代码、本地编译、调Skill、大模型分析落码、赛题、循迹、PVC、
  推荐算法、持续搜集开源、评估入库、选择性上传时必须使用；
  落码优先 refs/COMPILE_READY_WORKFLOW.md；搜集上传优先 refs/CONTINUOUS_INGEST.md。
---

# smart-car-dev · Skill + 大模型 → 可编译代码

## 核心能力

1. **按你的要求调取 Skill**，大模型分析后**落盘工程代码**  
2. **本地编译验证**（stub HAL / CMake），通过才算交付  
3. 赛题拆解、算法推荐（V/P/C/S）、整套结构  

## 启动方式（复制即用）

```text
用 smart-car-dev，按 COMPILE_READY_WORKFLOW：
组别/任务：……
输出到 generated/xxx，并本地编译通过。
```

## 强制文档

| 用户意图 | 打开 |
|----------|------|
| **Skill+大模型→可编译代码** | `refs/COMPILE_READY_WORKFLOW.md` ★ |
| 按赛题写代码 | `refs/FROM_PROBLEM_TO_CODE.md` |
| 算法+结构方案 | `refs/QUESTION_TO_ARCHITECTURE.md` |
| 算法代码索引 | `refs/ALGORITHMS_CODE_INDEX.md` |
| PVC/摄像头循迹 | `refs/VISION_LINE_PIPELINE.md` |
| 元素状态机 | `refs/ELEMENT_FSM_LIBRARY.md` |
| 电磁中线 | `refs/ELECTRO_MIDLINE.md` |
| Stanley/PP | `refs/STANLEY_PURE_PURSUIT.md` |
| 外设模块 | `refs/MODULES_BSP.md` |
| IMU/惯导 | `refs/IMU_INS_PLAYBOOK.md` |
| 逐飞/ADS 真机模板 | `refs/BOARD_TEMPLATES.md` |
| 逐飞核心板总表 | `refs/SEEKFREE_BOARDS.md` |
| CYT4BB7 双核 | `refs/CYT4BB7_DUAL_CORE.md` |
| 龙芯 LS2K | `refs/LS2K_LINUX_BOARD.md` |
| RK3588/RKNN | `refs/RKNN_DEPLOY.md` |
| 官方库本机镜像 | `board_libs/INDEX.md` |
| **持续搜集→自评→选择性上传** | `refs/CONTINUOUS_INGEST.md` ★ |
| 创意组 ROS2 骨架 | `refs/CREATIVE_ROS2_SKELETON.md` |

## 示例工程

- `generated/pvc_line_track/`（PVC 循迹，`build_host.bat`）
- `generated/board_seekfree_stub/`（逐飞单核通用）
- `generated/board_tc264_stub/` / `board_tc387_stub/` / `board_stc32_stub/` / `board_rt1064_stub/`（分板壳）
- `generated/board_cyt4bb7_stub/`（CYT4BB7 双核 IPC 示意）
- `generated/board_ls2k_host_stub/` / `board_rknn_host_stub/`（走马 Linux / 模型异步推理桥）
- `generated/creative_ros2_ws/`（创意组最小 ROS2 包骨架）

## 写码还要读

- `../smartcar-ml-optimize/refs/INGESTED_ALGORITHMS.md`  
- `../smartcar-ml-optimize/refs/CODE_TEMPLATES.md`  
- 赛道 `code_opensource/RECOMMENDED.md`  

## 模型训练（YOLO 等）

「训练大模型/检测模型」→ `smartcar-ml-optimize`（`workflow.md` / `DL_FROM_CORPUS.md`），  
Agent 生成训练脚本与配置；推理灌入仍回本 Skill 控制链路。

## 输出约定

未本地编译成功，不得声称可编译；板级用 `hal_*` 隔离。
