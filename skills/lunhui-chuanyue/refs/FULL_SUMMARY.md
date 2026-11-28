# 轮腿穿越 · 报告级知识摘要

综合同济 TU-Smart 开源、L-F-S-max/wheel-legged-robot、逐飞演示车模解析与立创开源提炼。

## 赛题要点

- 室外轮腿：两轮自平衡 + 腿式越障  
- 科目常见：高速巡线、雷区/旋转、单边桥、颠簸、台阶跳跃  
- 主控常见：**英飞凌 CYT4BB7**（双 M7）+ 可选 **CYT2BL3** 做无刷 FOC  
- 传感器：IMU 必备；视觉/GPS/TOF 按科目  

## 控制核心（开源共识）

```text
速度外环 → 腿部倾斜/零点（或俯仰目标）
俯仰角环 → 角速度内环 → 轮毂力矩
横滚环：单边桥调左右腿长
偏航环：差速转向
腿：四/五连杆 IK 或查表 → 舵机 PWM
跳跃：蓄力→伸腿→收腿→落地缓冲 状态机
视觉：CM7_1 处理，IPC 给控制核；不阻塞 1ms 平衡环
```

**关键点**：速度环尽量作用在**腿零点**，姿态双环专注抗扰（颠簸/落地），降低耦合。

算法锚点：`P02/C04/C06/C08/S01/S02` + 视觉 `V01–V03` 可选。

## 双核分工示例

| 核 | 职责 |
|----|------|
| CM7_0 | 1ms 平衡、腿、电机、科目状态机 |
| CM7_1 | 相机、IPM、元素识别、图传 |

## 安全

- 倾角超限倒地保护停转  
- 跳跃仅在触发条件满足时进入  
- 遥控急停  

## 对照仓 / 板级

- `code_opensource/`：boatchanting / L-F-S-max / lingkonglengfeng（含完整 Seekfree_CYT4BB 工程）  
- 官方库镜像：`../../smart-car-dev/board_libs/seekfree_CYT4BB7_Library/`（本 skill `code_opensource` 不再放空壳副本）  
- 双核指南：`../../smart-car-dev/refs/CYT4BB7_DUAL_CORE.md`  
- 逐飞板总表：`../../smart-car-dev/refs/SEEKFREE_BOARDS.md`  
- 平衡：`../../smart-car-dev/refs/IMU_INS_PLAYBOOK.md`  
- Host 双核骨架：`generated/board_cyt4bb7_stub/`
