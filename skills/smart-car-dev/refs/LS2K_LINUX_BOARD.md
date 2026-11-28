# 龙芯 LS2K / Linux 板级入口（走马观碑）

**不改变**现有 SCAFFOLD→RECOMMENDED→refs 流程；本页只作板级索引。  
非逐飞 MCU 库形态：用户态 Linux + OpenCV/NCNN/TFLM。

## 本机资料（已有，勿重复造目录）

| 类型 | 路径 |
|------|------|
| 赛道开源 | `../zouma-guanbei/code_opensource/`（Loognson、99Pai、smart2026、loongson-smart-car…） |
| 解析卡片 | `../references/repos/ls2k0300/`、`loognson/`、`loong-smartcar/` 等 |
| PVC 循迹可编译例 | `generated/pvc_line_track/` |
| 报告摘要 | `../zouma-guanbei/refs/FULL_SUMMARY.md` |

## 推荐职责划分

```text
相机采集线程/节点 → 循迹 + 碑文/检测（异步）
控制定时器/线程 → 只读最新 err/goal（带 stamp 超时）
底盘/电调驱动 → 限幅；退出顺序：停定时器→关电调
```

LS2K0300 逐飞例程要点（见 `references/repos/ls2k0300/review.md`）：  
视觉外环 + 角速度中环 + 轮速内环；`pit_callback` 跑后两环；停机先停 pit 再关驱动。

## 与 SEEKFREE_BOARDS 关系

MCU 板仍用 `SEEKFREE_BOARDS.md` / `board_libs/`。  
走马优先读本页 + zouma skill，不要去 `board_libs` 找 LS2K 空目录。
