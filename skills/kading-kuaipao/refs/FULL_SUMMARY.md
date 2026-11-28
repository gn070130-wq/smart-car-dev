# 卡丁快跑 · 完整汇总

## 架构
```
遥控/发车 → 科目状态机
  ├ 科目一：定距录径 → Flash 点列 → CTE 回放转向
  ├ 科目二/语音：ASR → 灯光/鸣笛/点阵
  └ 跟随：视觉目标 → 横向/纵向控制 → 转向电机闭环 + 驱动/制动
```
主控常见 TC387 四核分工（感知/控制/通信/UI）或双 TC264。  
板库：`../../smart-car-dev/board_libs/seekfree_TC387_Library`。

## 文件结构
逻辑：`vision_* / path_record_* / cte_* / steer_pid_* / speed_gate_* / voice_* / task_fsm_*`  
实码锚点（任选其一，勿找不存在的相对 `code_tree/`）：
- 开源仓：`code_opensource/21届_卡丁快跑__gitee__why137a__cading-code/km13_tc387/`
- 报告解压：仓库根目录 `knowledge_base_kading/code_tree/km13/`（与 skill 并列）
- 板库：`../../smart-car-dev/board_libs/seekfree_TC387_Library`

## 算法
见 `ALGORITHMS.md`：CTE、转向绝对编码器 PID、速度门控、视觉跟随、语音匹配、多科目 FSM。

## 风险
语音断网、跟丢目标、转向间隙振荡、制动延迟、录径里程滑移。
