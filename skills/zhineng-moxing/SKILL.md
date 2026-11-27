---
name: zhineng-moxing
description: >-
  全国大学生智能汽车竞赛「智能模型/人工智能模型」完整内置知识库。含 TC264+RK3588 NPU、
  YOLO/分割/量化部署、上下位机串口、阿克曼 PID。用户提到智能模型、人工智能模型、
  RK3588、NPU、YOLO 车载推理时必须使用；直接读本目录 refs/ 与 corpus/。
---

# 智能模型 Skill（全文已安装）

内置：`refs/` + `corpus/`

摘要：RK 侧推理 → 串口偏差 → TC264 控制；核心是检测/分割量化与实时闭环。  
调试：模型 FPS → 协议 → 低速 PID → 提速 → 掉帧策略。  
开发模式已开启。
