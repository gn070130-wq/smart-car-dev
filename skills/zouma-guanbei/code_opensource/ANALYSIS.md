# 走马观碑 · 结构分析

对照仓（优先）：`届次待核_走马观碑__gitee__amireux_fish__Loognson`  
主控/板级：龙芯 LS2K / Linux  
默认算法链：`V06/V10→P07→S01→C01 + S03`  
对外接口：`frame → text/pose/stamp → nav_goal → chassis_cmd`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
linux_app/
  vision/ocr_or_det.*     # 碑文/标牌
  nav/planner.*           # 点到点/栅格
  task/stele_fsm.*        # 寻碑→识别→记录→下一站
  chassis/driver_node.*   # 串口到底盘
  cfg/params.yaml
见 smart-car-dev/refs/LS2K_LINUX_BOARD.md
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `ocr_det` | frame | text, pose, stamp |
| `stele_fsm` | text, pose | nav_goal, log |
| `planner` | goal, map | path |
| `chassis` | cmd | ack / fault |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

OCR误识、定位漂移、Linux 实时性；推理异步不堵底盘环。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/届次待核_走马观碑__gitee__amireux_fish__Loognson`

```text
project/
  user/   main.cpp CMakeLists.txt cross.cmake build.sh
  code/   get_image.* deal_image.* deal_model.* direction.* device.* menu.*
  model/  loong_cnn_model_simple.tflite / .h
libraries/  zf_*
```

映射：`deal_model`→V06/V10；`direction`→导航/底盘；Linux 交叉编译见 `cross.cmake`；host 示意 `generated/board_ls2k_host_stub/`。
