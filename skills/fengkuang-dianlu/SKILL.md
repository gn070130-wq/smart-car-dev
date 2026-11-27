---
name: fengkuang-dianlu
description: >-
  全国大学生智能汽车竞赛「疯狂电路」完整内置知识库。含 TC264、总钻风灰度、差速循迹、
  边线/元素状态机、PID、负压涵道。用户提到疯狂电路、闪电鼠、差速循迹、负压吸附、
  TC264 室内竞速时必须使用；直接读本目录 refs/ 与 corpus/，不要要求打开外部文件夹。
---

# 疯狂电路 Skill（全文已安装）

## 内置路径
| 路径 | 用途 |
|------|------|
| `refs/FULL_SUMMARY.md` | 完整汇总 |
| `refs/LEARNING_SUMMARY.md` | 学习摘要 |
| `refs/INDEX.md` | K1–K10 / A1–A8 |
| `refs/ALGORITHMS.md` | 算法卡 |
| `refs/FILE_STRUCTURE.md` | 逻辑结构 |
| `refs/COMPARE_AND_RISKS.md` | 对比风险 |
| `refs/MINE_SIGNALS.md` | 信号表 |
| `refs/INGEST_LOG.md` | 解析日志 |
| `corpus/` | 报告正文 |

## 赛道摘要
室内视觉循迹；**TC264** + 灰度摄像头 + 差速 + 常配负压；阈值边线→元素机→PD/速度环。

## 调试顺序
曝光阈值 → 边线 → 低速 PD → 速度环 → 元素表速 → 负压 → 提速

## 开发模式
可补代码/重构/改算法；原+改后+说明+测试。
