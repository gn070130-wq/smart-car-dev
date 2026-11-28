# 智慧工厂 · 结构分析

对照仓（优先）：`21届_智慧工厂__github__wayfarer-nova__smart-factory-ucar-2026`  
主控/板级：巡线小车 + 视觉元素（锥桶/灯）  
默认算法链：`V01/V05→S01(环岛/锥桶/灯)→C01/C02`  
对外接口：`line_err, traffic_state → steer,speed`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
firmware/ 或 ros2/
  perception/line_* traffic_*
  planning/factory_fsm.*
  control/steer_speed.*
  actuators/safety.*
公开仓偏薄：优先 SOFTWARE_SKELETON + 本 ANALYSIS 搭模块。
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `line` | img/adc | err |
| `traffic` | roi | light_state |
| `factory_fsm` | err, light, cone | v_gate, action |
| `steer` | err | delta |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

元素误检、环岛丢线；公开对照少，先开环循迹再叠灯。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_智慧工厂__github__wayfarer-nova__smart-factory-ucar-2026`（偏薄）

```text
light_flow/  config/  scripts/  docs/  requirements.txt
```

源码少：循迹/灯控按 ANALYSIS 逻辑名自建；也可用 `generated/creative_ros2_ws/` 起盘。
