# 整合说明：smart-car-dev × 九赛道 × ml-optimize

来源仓库：https://github.com/gn070130-wq/smart-car-dev  
本地技能：`../smart-car-dev/`（轻量：SKILL、references、scripts、53 份走马观碑 corpus、多仓解析；不含 upstream 全量归档）

## 能力合并后怎么选

| 你要做的事 | 优先读 |
|------------|--------|
| 某国赛组别硬件/算法共识 | `smartcar-competition` → 组别 skill |
| 按已灌入算法 ID 写模块 | `INGESTED_ALGORITHMS.md` + `CODE_TEMPLATES.md` |
| 巡线/PID/LADRC/状态机实车调试 | `smart-car-dev` → control/vision/debug |
| NCNN/TFLite 输入输出契约、冒烟 | `smart-car-dev/references/multi-repo/model-runtime.md` + scripts |
| 检索走马观碑 53 报告 | `python ../smart-car-dev/scripts/search_reports.py` |
| 借鉴 ZouMaGuanBei / 13 仓解析 | `smart-car-dev/references/repos/` |
| YOLO/RKNN/量化与九赛道 DL 链 | 本 skill `DL_FROM_CORPUS.md` + 组别 `zhineng-moxing` |

## 写代码时的统一约束（两边对齐）

1. 以**当前工程实测**为准，报告/开源仅为参考  
2. 不把报告 PID/引脚/阈值当默认写入电机配置  
3. 改代码：最小模块 + 【原→改→说明→测试】  
4. 控制：单位、符号、周期、积分复位、超时一起核  
5. 模型：分类≠检测；logits≠概率；CPU 冒烟≠板端性能  
6. 无板卡时做离线验证并列出实车待验项  

## 常用命令（本机路径）

```text
python ".cursor/skills/smart-car-dev/scripts/search_reports.py" --query "角速度" --ids 02,13,53
python ".cursor/skills/smart-car-dev/scripts/inspect_ncnn_contract.py" "model.param" --input in0 --output out0
python ".cursor/skills/smart-car-dev/scripts/analyze_telemetry.py" "run.csv" --expected-period-ms 5
python ".cursor/skills/smart-car-dev/scripts/smoke_tflite.py" "model.tflite"
```
