# RK3588 / RKNN 部署入口（智能模型）

**不改变** COMPILE_READY / 赛道 skill 主流程；本页补 NPU 部署索引。

## 本机对照（已有开源）

| 仓 | 用途 |
|----|------|
| `../zhineng-moxing/code_opensource/…PaddleYOLO-RKNN` | 检测→RKNN |
| `../zhineng-moxing/code_opensource/…X-SmartCar-AI-Model-Multitask` | 多任务 |
| 农场模型壳 | `../zhihui-nongchang/…smart_farm_models`（无 datasets） |

赛道摘要：`../zhineng-moxing/refs/FULL_SUMMARY.md`

## 推荐流水线

```text
训练(Paddle/YOLO) → 导出 ONNX → RKNN-Toolkit 量化
→ 板上 rknn_runtime 推理 → 偏差/目标结构体
→ UART/共享内存 → 下位机 PID（TC264 等）
```

## 约束

- 推理异步；控制环只读带 `stamp` 的最新结果，超时降速/直行  
- 输入尺寸/色序与训练一致；NMS/置信度实车重标  
- 下位机协议定长+校验；禁止在 ISR 里跑 NPU  

训练脚本生成仍走 `smartcar-ml-optimize`；车控落码仍走 `COMPILE_READY_WORKFLOW`。
