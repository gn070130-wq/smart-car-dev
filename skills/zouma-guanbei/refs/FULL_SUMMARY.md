# 走马观碑 · 报告级知识摘要

综合本机开源仓、赛道规则摘要与 `smart-car-dev/references` 多仓解析提炼。

## 赛题要点

- 室内 **PVC 赛道** 跑圈 + **石碑/碑文** 相关任务（识别/停靠/记录视规则）  
- 常见平台：**龙芯 LS2K** / Linux 用户态 + 相机；或 MCU+上位视觉  
- 元素：弯道、十字、环岛、坡道、路障等（与视觉循迹同类）  
- 视觉任务与底盘控制建议**解耦**（推理异步，控制定周期）  

## 默认软件栈

```text
相机帧 → 循迹(PVC边线/中线) 与/或 检测/OCR(V06/V10)
      → 任务状态机 S01（寻碑→识别→下一目标→完赛）
      → 底盘 C01（差速/阿克曼）+ 超时安全 S03/S05
IMU/里程 → 辅助定位；丢视觉降速
```

## PVC 循迹

直接复用总控：

- `../../smart-car-dev/refs/VISION_LINE_PIPELINE.md`  
- `../../smart-car-dev/refs/ELEMENT_FSM_LIBRARY.md`  
- 示例工程：`generated/pvc_line_track/`  

注意反光/接缝：底部阈值、边线跳变限制、丢线降速。

## 碑文/目标

- 检测框或 OCR；多帧投票 V09  
- 结果带 `stamp`；超时不更新控制目标  
- Linux 侧可用 ROS2 / 自研节点；经 UART/共享内存下发 `err/goal`

## 搭建顺序

1. PVC 循迹闭环可跑圈  
2. 相机预览与存图  
3. 碑点检测最小模型或传统 CV  
4. 任务状态机串起来  
5. 日志与调参  

对照：`code_opensource/RECOMMENDED.md`  
龙芯：`../../smart-car-dev/references/repos/`（`ls2k0300` / `loognson` 等）  
板级入口：`../../smart-car-dev/refs/LS2K_LINUX_BOARD.md`  
PVC 示例：`generated/pvc_line_track/`
