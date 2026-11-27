# 智能模型 · 完整汇总

## 架构
`摄像头 → RK3588 NPU(YOLO/分割/OCR) → UART → TC264(PID/电机舵机)`

## 逻辑文件
`npu_infer_* / yolo_* / seg_* / uart_proto_* / steer_pid_* / speed_pid_* / calib_*`

## 算法
检测、分割、量化、偏差提取、串级/PID、标定、上下位机协议。

## 风险
推理超时掉帧、量化漏检、串口阻塞控制、阿克曼高速失稳。
