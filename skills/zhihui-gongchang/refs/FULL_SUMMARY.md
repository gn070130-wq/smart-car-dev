# 智慧工厂（创意）· 知识摘要

流程不变：SCAFFOLD → RECOMMENDED → 本 refs。

## 要点

- 厂区巡线 + 环岛/锥桶/红绿灯等元素常见  
- 对照：`code_opensource/` 内 `smart-factory-ucar-2026`  
- PVC/循迹可直接复用总控视觉管线与 `generated/pvc_line_track`

## 软件链路

```text
灰度/彩色相机 → V01/V05 循迹与检测
→ S01 元素 FSM（环岛/灯/锥桶）
→ C01/C02 转向速度
→ S03 安全
```

## 搭建顺序

1. 循迹可跑直道弯道  
2. 元素单开调试（灯/锥）  
3. 状态机串联  
4. 厂区任务点（若有）  

详见：`VISION_LINE_PIPELINE.md`、`ELEMENT_FSM_LIBRARY.md`。
