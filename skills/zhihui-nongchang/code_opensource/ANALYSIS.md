# 智慧农场 · 结构分析

对照仓（优先）：`21届_智慧农场__github__indulgeuuu-del__smart_farm_models`  
主控/板级：巡检/作业车 + 模型可选  
默认算法链：`V感知 → S01任务 → C05 → S03`  
对外接口：`crop/row_target → task → chassis`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
app/
  perception/row_or_crop.*
  task/farm_fsm.*
  control/omni_or_diff.*
  models/                 # 可选部署脚本
骨架：SOFTWARE_SKELETON + CREATIVE_ROS2_SKELETON
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `row_sense` | img | lateral_err |
| `farm_fsm` | mission | action |
| `control` | err | cmd |
| `safety` | estop | hold |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

田间打滑、模型域偏移；先沿垄循迹再叠作业。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_智慧农场__github__indulgeuuu-del__smart_farm_models`

```text
configs/  deploy/  env/  assets/
DATA_MODEL_PROVENANCE.md  RELEASE_MANIFEST.json
```

偏模型/部署；车控仍按 ANALYSIS + `CREATIVE_ROS2_SKELETON` / `SOFTWARE_SKELETON` 自建。
