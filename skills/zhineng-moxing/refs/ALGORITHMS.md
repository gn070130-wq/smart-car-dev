# 智能模型 · 算法映射

| ID | 模块 | 说明 |
|----|------|------|
| A1/V05 | YOLO 检测 | NPU 推理框 |
| A2 | 语义分割 | mask→中线/偏差 |
| A3 | 量化部署 | PTQ→RKNN/ONNX |
| A4 | 偏差提取 | 扫线/质心→err |
| A5/C01 | PID | 下位机转向速度 |
| A6 | OCR/Tag | 辅助任务 |
| A7 | 协议 | 定长帧+校验+超时 |

默认链：`检测/分割 → 偏差 → UART → PID`。  
详部署：`../../smart-car-dev/refs/RKNN_DEPLOY.md`。
