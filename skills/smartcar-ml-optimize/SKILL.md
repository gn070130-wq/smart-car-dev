---
name: smartcar-ml-optimize
description: >-
  智能车竞赛「大模型+深度学习」算法学习与优化助手。已灌入九赛道报告提炼的算法总表、
  赛道接口地图、代码模板与 DL 部署经验，用于后续调用完善代码。用户提到大模型优化、
  深度学习调优、YOLO、分割、量化、RKNN、NPU、算法学习、完善代码、按已学算法写模块时必须使用；
  先读本 skill 的 refs 灌入库，再打开对应赛道 skill。
---

# 智能车 · 大模型 + 深度学习算法优化 Skill

已将上传文档学到的内容 **灌入** 本 skill，供后续直接调用写/改完善代码。  
飞檐走壁 / 走马观碑不在安装范围。

## 灌入库（写代码必读）

| 文件 | 用途 |
|------|------|
| `refs/INGESTED_ALGORITHMS.md` | 九赛道算法总表 V/P/C/S + DL 链 |
| `refs/TRACK_ALGORITHM_MAP.md` | 赛道默认模块链与接口 |
| `refs/CODE_TEMPLATES.md` | 可替换代码模板 T1–T8 |
| `refs/DL_FROM_CORPUS.md` | 报告侧 DL/NPU/混合策略 |
| `refs/workflow.md` | 学习→优化→部署 |
| `refs/dl-playbook.md` | 检测/分割/量化速查 |
| `refs/INTEGRATION_SMART_CAR_DEV.md` | 与 GitHub `smart-car-dev` 整合分工 |

证据不足时再读并列赛道 `../<track>/refs/` 与 `corpus/`。  
工程调试 / NCNN·TFLite / 走马观碑开源解析 → `../smart-car-dev/`。

## 调用写代码时的强制步骤

1. 判定组别 → 打开 `TRACK_ALGORITHM_MAP.md` 对应链  
2. 从 `INGESTED_ALGORITHMS.md` 取算法 ID（如 V06、C04）  
3. 用 `CODE_TEMPLATES.md` 对应模板改写成用户工程语言/接口  
4. DL 相关读 `DL_FROM_CORPUS.md`：异步推理、新鲜度、量化回归  
5. 输出【原始代码 → 修改后 → 说明 → 测试】；无原码则给可粘贴模块 + 假设接口  
6. 约束：控制环非阻塞；限幅；超时安全；关键点调试打印  

## 快速改版 vs 长期

- **快速**：阈值/conf/imgsz/NMS、PID、门控、超时、投票窗口  
- **长期**：解耦 DetectTarget 接口、推理队列、录包回归、状态机规范化  

## 输出模板

```
【组别】…
【算法ID】V../P../C../S..（出自 INGESTED_ALGORITHMS）
【现状→目标】…
【方案】快速 / 长期
【代码】原→改→说明→测试
【风险】算力/延迟/赛场故障
```

## 九赛道并列路径

`yanguo-liuhen` · `danche-dingxiang` · `feiyue-leiqu` · `fengkuang-dianlu` · `kading-kuaipao` · `lunhui-chuanyue` · `mayi-banjia` · `zhineng-moxing` · `zhineng-shijue` · `smart-car-dev`
