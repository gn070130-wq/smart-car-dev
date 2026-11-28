# 智能模型 · 结构分析

对照仓（优先）：`21届_人工智能模型__github__cozylove-girl__X-SmartCar-AI-Model-Multitask`  
主控/板级：RK3588(NPU) + 下位机（常 TC264）  
默认算法链：`V06/V07→RKNN→V09→S03→C01`  
对外接口：`RK(err/type/stamp) → MCU(steer,speed)`

## 推荐目录（逻辑名，接到真实仓时改名）

```text
rk3588/
  infer_rknn.*            # YOLO/Seg 异步
  post_vote.*             # V09 多帧
  uart_down.*             # 下发结果帧
mcu/
  control/steer_speed.*
  safety/link_watchdog.*
见 RKNN_DEPLOY.md；模板 generated/board_rknn_host_stub/
```

## 模块接口

| 模块 | 输入 | 输出 |
|------|------|------|
| `rknn_infer` | rgb | boxes/mask, stamp |
| `vote` | raw det | stable type/err |
| `uart_down` | stable | mcu_frame |
| `mcu_ctrl` | frame | steer,speed / safe |

## 主环约定

1. 感知非阻塞；推理/OCR/NPU **异步**，带 `stamp` 与新鲜度。  
2. 状态机只切模式与门控速度，不直接写裸 PWM（经 control/actuators）。  
3. 上电默认安全态；超时、丢线、掉链 → `safety` 收口。  

## 风险与联调

NPU 延迟堵环、模型过拟合、帧新鲜度；推理必须异步。

落码：`../../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
算法 ID：`../../smartcar-ml-optimize/refs/TRACK_ALGORITHM_MAP.md`

## 真实对照目录（本机 ★ 仓）

仓：`code_opensource/21届_人工智能模型__github__cozylove-girl__X-SmartCar-AI-Model-Multitask`

```text
PaddleDetection/          # 训练框架
main_multitask_ppyoloe.ipynb
实验结果/  技术路线与传承说明.md
AI_STUDIO_上传训练指南.md
```

部署链另对照 `Dengdxx/PaddleYOLO-RKNN`（INDEX）。  
映射：训练→导出→RKNN；下位机控制用 `generated/board_rknn_host_stub/` 新鲜度约定。
