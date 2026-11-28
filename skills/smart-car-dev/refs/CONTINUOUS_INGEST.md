# 持续搜集 → 自评 → 选择性上传

目标：不断发现智能车竞赛相关开源/报告/板级资料，**先评估再入库**，只把你勾选的内容同步到 GitHub（`gn070130-wq/smart-car-dev`）。

## 你的节奏（推荐）

```text
每周 / 每有新发现
  → 搜集+评估（自动）
  → 你看 REPORT，改 decisions
  → apply（只写元数据）
  → 你点头后再 push GitHub
```

**禁止**：未评估直接大体积 push；`board_libs` 全文与完整 `code_opensource` 树默认不上 Git。

## 本机命令

工作区根：`智能车skill/`

```powershell
# 一键：搜集 + 评估（不入库、不上传）
python _kb_raw/collect/collect_run.py --track 飞檐走壁
python _kb_raw/collect/collect_run.py --all --limit 20

# 分步（等价）
python _kb_raw/collect/collect_discover.py --track 轮腿穿越
python _kb_raw/collect/collect_evaluate.py --batch <batch_id>

# 可选：按 INDEX 去重后自动勾选高分 accept_meta（仍可再手改）
python _kb_raw/collect/auto_select.py --batch <batch_id> --min-score 20 --max-accept 35

# 编辑 decisions/<batch_id>.json 的 verdict 后：
python _kb_raw/collect/collect_apply.py --batch <batch_id> --dry-run
python _kb_raw/collect/collect_apply.py --batch <batch_id>

# 上传 GitHub（仅元数据/refs；在 smart-car-dev-repo）
cd _kb_raw\smart-car-dev-repo
git add skills
git commit -m "Ingest approved collect batch <id>"
git -c http.version=HTTP/1.1 push origin HEAD
```

## 评估维度

| 分 | 含义 |
|----|------|
| A 相关度 | 组别 `groups` / 赛题匹配（已严格按组过滤） |
| B 可编译性信号 | 含代码工程、语言、git 可访问 |
| C 新鲜度 | pushed_at / 届次 |
| D 重复度 | 是否已在 INDEX |
| E 体积风险 | datasets/权重 → 建议只链 URL |

**verdict**（你改）：

| 值 | 含义 |
|----|------|
| `accept_meta` | 只写 INDEX/RECOMMENDED 链接与备注（推荐默认） |
| `accept_clone` | 本机可再 clone；仍默认不进 Git 大树 |
| `defer` | 下次再看 |
| `reject` | 丢弃 |

机器只给 `suggest_verdict`，**默认不自动 accept**（见 decisions 草稿）。

## Agent 话术

```text
按 CONTINUOUS_INGEST：搜集「xxx」新开源，只生成评估报告，不要 apply、不要 push。
```

```text
按 CONTINUOUS_INGEST：batch XXX 我已改好 decisions，执行 apply（仅 accept_meta）并 push GitHub。
```

## 目录

```text
_kb_raw/collect/
  collect_run.py              # 一键搜集+评估
  collect_discover.py / collect_evaluate.py / collect_apply.py
  inbox/<batch>.json
  eval/<batch>_REPORT.md
  decisions/<batch>.json      # ★ 你改这里
  applied/<batch>.log
  README.md
```

## 扩充发现源

- 主源：`_kb_raw/discovery.json`（可继续追加仓库条目后重跑 discover）
- 手工：往 inbox JSON 的 `candidates` 塞 URL，再 `collect_evaluate.py`
- 安装：`install_opensource_by_track.py` 仅在你明确 `accept_clone` 且要本机树时用
