# 蚂蚁搬家 · 完整汇总

## 架构
`视觉目标 → 全向运动学+PID → 推杆推物 → 灰度/边线确认 → 返回；可选主从无线协同`

## 文件结构（逻辑）
`vision_* / kinematics_omni_* / pid_speed_* / imu_ins_* / collab_uart_* / task_fsm_* / path_dijkstra_*`

## 算法清单
目标检测/色块、视觉伺服、全向逆解、速度 PID、Mahony 姿态、Dijkstra/可视图、S 曲线、主从帧协议。

## 风险
丢目标、推偏、越界、双车失步、MicroPython 实时性不足、反光误检。
