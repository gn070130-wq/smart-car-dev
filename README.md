> **存储策略**：Skill 正文以本 GitHub 仓库为权威源；本机 Cursor 工作区可另有完整开源/`board_libs` 镜像（体积大，默认不入 Git）。

# 智能车开发与调试 Skill（竞速十一组 + 创意组）

用于 Codex/Cursor：代码分析与重构、视觉巡线、目标识别、状态机、PID/LADRC、硬件联调、日志与模型检查；并含国赛**竞速十一组 + 创意五组**知识库、逐飞/LS2K/RKNN 板级文档，以及 `generated/` 可编译 stub。

仓库：https://github.com/gn070130-wq/smart-car-dev

资料含走马观碑参赛报告提取与固定版本开源解析（见 `references/`）。文件数量不代表逐行验证或实车成功率。  
`references/corpus/*.txt` 保持原样（CRLF）；改换行会破坏校验哈希。

## 从这里开始

| 文档 | 用途 |
|------|------|
| [SKILL.md](SKILL.md) | 统一入口与调试路由 |
| [skills/README.md](skills/README.md) | 赛道/总控/ML 目录说明 ★ |
| [skills/smartcar-competition/SKILL.md](skills/smartcar-competition/SKILL.md) | 未定组别时分流 |
| [skills/smart-car-dev/PERFECT_SKILL_GUIDE.md](skills/smart-car-dev/PERFECT_SKILL_GUIDE.md) | 使用指南 |
| [skills/smart-car-dev/refs/COMPILE_READY_WORKFLOW.md](skills/smart-car-dev/refs/COMPILE_READY_WORKFLOW.md) | 落可编译代码 |
| [generated/](generated/) | PVC / 分板 stub 源码 |
| [references/multi-repo/overview.md](references/multi-repo/overview.md) | 多仓解析 |
| [references/source-map.md](references/source-map.md) | 报告索引 |

## 安装和使用

把本仓库放入技能目录，文件夹名 `smart-car-dev`，保证根目录有 `SKILL.md`。

```text
使用 $smart-car-dev，按 COMPILE_READY 为「疯狂电路」生成循迹工程到 generated/xxx 并说明如何本地编译。

使用 smartcar-competition 定组后，用飞檐走壁 skill 按 SCAFFOLD 搭控制软件。

使用 $smart-car-dev，检查 NCNN 输入输出与标签映射是否与目标板一致。
```

## 目录

```text
SKILL.md                 统一入口与调试路由
skills/                  竞速十一组 + 创意组 + smart-car-dev + competition + ml-optimize
generated/               可编译 host 骨架（无 build/ 产物）
agents/                  技能展示信息
assets/                  车模配置、模型契约、日志列模板
references/              走马观碑专题、多仓解析、报告正文
scripts/                 报告检索、源码索引、遥测和模型检查
tools/validate.py        仓库完整性与工具行为检查
```

常用命令：

```bash
python scripts/search_reports.py --query "角速度" --ids 02,13,53
python scripts/lookup_open_source.py --repo-id runaccm2025 --query consumer
python scripts/read_source.py --repo-id smart2026 --path smartCar/code/yolov5_detect.cc
python scripts/inspect_ncnn_contract.py model.param --input in0 --output out0
python scripts/analyze_telemetry.py run.csv --expected-period-ms 5
python scripts/smoke_tflite.py model.tflite
python -X utf8 tools/validate.py
```

## 推送后同步说明

本仓 **不含** 完整 `board_libs/seekfree_*` 与各赛道 `code_opensource` 全文；板库见 `skills/smart-car-dev/board_libs/INDEX.md` 上游链接，开源全文在本地 Cursor 工作区。

验证范围：重点静态分析与工具冒烟；未声称全路径实车/训练验证。权利说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
