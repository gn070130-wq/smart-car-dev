# 智能视觉 · 结构分析

对照仓（优先）：`21届_人工智能视觉__github__Hey9990__SmartCar-21AIVision`  
主控/板级：RT1064（俯视+第一人称常见）  
默认算法链：`V08→P07→P08→S01→C05→C01 + V09`  
对外接口：`grid,path,box_plan → omni_cmd`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/ 或 HIL/
  map_vision_*            # 俯视建图
  fpv_vision_*            # 第一人称
  a_star_* / sokoban_*    # 规划/推箱
  omni_pid_* fsm_*
  bsp/RT1064
板库：smart-car-dev/board_libs/seekfree_RT1064_Library
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `map_vision` | top_cam | grid |
| `sokoban` | grid, boxes | box_plan / fail |
| `a_star` | grid, goal | path |
| `omni_pid` | path_err | vx,vy,w |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

透视误差、推箱无解、双视觉不同步；仿真到实车标定。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_人工智能视觉__github__Hey9990__SmartCar-21AIVision`

```text
SeekFree_RT1064_Opensource_Library/  project/ libraries/
OpenART-code/   map*.txt 调试
Example/        Coreboard / Motherboard / OpenART 例程
Report/         IMU/DMA/串口调试笔记
21届…资料/      硬件与麦轮方案 md
uart1_map_parser.c  sokoban_visual.html
```

映射：建图/推箱在 OpenART + 上位解析；底盘 RT1064 `project/`；对照 `generated/board_rt1064_stub/`。
