# 智能车开发与调试 Skill

用于 Codex 的智能车竞赛开发技能，支持代码分析、整套重构、视觉巡线、目标识别、状态机、PID/LADRC、硬件联调、运行日志和模型调用检查。

资料来自53份走马观碑参赛报告，以及固定版本的开源项目解析。本轮新增13个仓库地址的结构与算法梳理，保存714份文件/归档成员的校验记录与2682个函数定义候选；另保留之前的ZouMaGuanBei解析。镜像和同提交来源明确标注，文件数量不代表逐行验证或实车成功率。

`references/corpus/*.txt` 按提取结果原样保存（CRLF 换行），其 SHA256 与字符数记录在 `corpus/index.json`；用编辑器转换换行或重新排版会同时破坏 53 份正文的校验，请勿改动这些文件。

## 从这里开始

- [技能入口](SKILL.md)
- [多仓库结构与算法集合](references/multi-repo/overview.md)
- [代码问题、离线反例与误报排除](references/multi-repo/findings.md)
- [深度学习模型调用与验证](references/multi-repo/model-runtime.md)
- [53份报告来源索引](references/source-map.md)
- [原始来源与验证范围](references/provenance.json)

本仓库保存技能、解析、报告正文、实际应用源码、模型文件和工具。13份去重的完整上游快照（包括上游提供的内核、工具链、SDK和硬件资料）及53份原始报告保存在同一仓库的Release附件中；本机已接入技能离线目录。查看[离线源码与完整附件](references/offline-sources.md)。这不是只有上游网址的索引；上游未提供的文件仍不会被虚构补齐。仓库按用户明确选择保持公开。

## 安装和使用

把本仓库放入 Codex 技能目录，文件夹名使用 `smart-car-dev`，确保 `SKILL.md` 直接位于该文件夹下。已有同名技能时，先比较版本，再更新需要的文件。

调用示例：

```text
使用 $smart-car-dev，分析当前工程直道蛇行的原因，检查视觉前瞻、角速度环和轮速环，并完成必要修改与验证。

使用 $smart-car-dev，参考多仓库解析重构当前智能车，保留硬件接口，先梳理数据流、任务周期和状态机。

使用 $smart-car-dev，检查这个NCNN模型的输入输出和标签映射是否与目标板识别代码一致。
```

## 目录

```text
SKILL.md                 技能指令和按任务路由
agents/                  技能展示信息
assets/                  车模配置、模型契约、日志列模板
references/              分主题知识、报告正文、项目解析与证据
scripts/                 报告检索、源码索引、遥测和模型检查
tools/validate.py        不依赖板卡的仓库完整性与工具行为检查
assets/upstream/         可直接浏览的实际应用源码及模型
assets/upstream-archives/ 完整快照，本机已安装；GitHub中位于Release附件
```

常用命令：

```bash
python scripts/search_reports.py --query "角速度" --ids 02,13,53
python scripts/lookup_open_source.py --repo-id runaccm2025 --query consumer
python scripts/read_source.py --repo-id smart2026 --path smartCar/code/yolov5_detect.cc
python scripts/inspect_ncnn_contract.py model.param --input in0 --output out0
python scripts/analyze_telemetry.py run.csv --expected-period-ms 5
python scripts/smoke_tflite.py model.tflite
python tools/download_source_archives.py
```

除TFLite冒烟需要NumPy及兼容解释器外，检索和静态检查工具只使用Python标准库。TFLite工具只验证CPU合成输入下的分配与Invoke，不验证识别准确率、TFLM算子覆盖或龙芯时延。

## 后续优化流程

1. 在分支中修改技能入口、相关专题参考或辅助工具。
2. 新增项目时记录仓库URL、固定提交、文件哈希、实际调用路径和验证范围；对照已有来源避免重复计入。
3. 问题结论区分已复现、静态成立、条件风险与未验证。保留失败输入或最小反例，不只保存结论。
4. 执行校验，再提交和推送：

```bash
python -X utf8 tools/validate.py
git add .
git commit -m "Improve smart car development guidance"
git push
```

本仓库与已安装技能是两个副本。本次更新已同步技能、元数据、实际应用源码与离线归档入口；以后改动仍需同步。归档缓存较大，保存在D盘并通过目录联接接入技能；迁移电脑时用下载工具恢复归档，不把空联接误认为文件已安装。

验证脚本检查格式、来源/符号关系、53份正文哈希、相对链接，以及检索、遥测、NCNN契约工具的关键行为。目标板编译、模型样本对照和实车试跑仍按具体改动另行执行。

## 当前验证范围

已进行重点源码静态分析、部分数值/图契约反例，以及一份TFLite模型的本机CPU冒烟。没有完成所有项目的全路径审计、目标板构建、训练、识别精度或整车比赛验证。

资料与引用的权利说明见 [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)。
