# skills/ — 国赛竞速十一组 + 创意组 + 总控 + ML

本目录是 Agent 调用时的赛道/总控入口；**GitHub 为本仓权威副本**。  
远程：https://github.com/gn070130-wq/smart-car-dev

## 怎么调用（给 Agent / 用户）

1. **未指定组别** → `smartcar-competition/SKILL.md` 定组  
2. **定组后搭软件/改代码** → 该赛道 `SCAFFOLD` → `RECOMMENDED` → `ANALYSIS` → `refs/FULL_SUMMARY` + `ALGORITHMS`  
3. **要可编译代码** → `smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`，输出到仓库根 `generated/`  
4. **算法 ID / 模板** → `smartcar-ml-optimize`  
5. **上板** → `smart-car-dev/refs/BOARD_TEMPLATES.md` + `SEEKFREE_BOARDS.md`（或 LS2K / RKNN 专页）

## 目录一览

| 目录 | 说明 |
|------|------|
| `smart-car-dev/` | 总控：可编译流程、算法索引、逐飞/CYT4BB7/LS2K/RKNN 文档；`board_libs` 仅 INDEX |
| `smartcar-competition/` | 竞速+创意分流表 |
| `smartcar-ml-optimize/` | 算法总表、代码模板、DL/量化 |
| 竞速十一组 | 见下表 |
| 创意五组 | 见下表 |

### 竞速十一组

| Skill | 组别 |
|-------|------|
| `feiyan-zoubi` | 飞檐走壁 |
| `fengkuang-dianlu` | 疯狂电路 |
| `mayi-banjia` | 蚂蚁搬家 |
| `feiyue-leiqu` | 飞跃雷区 |
| `zouma-guanbei` | 走马观碑 |
| `yanguo-liuhen` | 雁过留痕 |
| `zhineng-shijue` | 智能视觉 |
| `zhineng-moxing` | 智能模型 |
| `kading-kuaipao` | 卡丁快跑 |
| `lunhui-chuanyue` | 轮腿穿越 |
| `danche-dingxiang` | 单车定向 |

### 创意组

| Skill | 组别 |
|-------|------|
| `zhihui-yiliao` | 智慧医疗 |
| `zhihui-gongchang` | 智慧工厂 |
| `zhihui-jiuyuan` | 智慧救援 |
| `zhihui-chengshi` | 智慧城市 |
| `zhihui-nongchang` | 智慧农场 |

每个赛道通常含：`SKILL.md`、`code_opensource/{SCAFFOLD,RECOMMENDED,INDEX,ANALYSIS}.md`、`refs/`（报告级摘要与算法映射）。  
**整仓开源源码**默认在本机 Cursor 工作区，不全部推入本 Git（避免体积过大）。

## 可编译示例（仓库根 `generated/`）

| 目录 | 用途 |
|------|------|
| `pvc_line_track/` | PVC 循迹 host 示例 |
| `board_seekfree_stub/` | 逐飞单核通用骨架 |
| `board_tc264_stub/` | TC264 / ADS |
| `board_stc32_stub/` | STC32 |
| `board_rt1064_stub/` | RT1064 |
| `board_cyt4bb7_stub/` | CYT4BB7 双核 IPC 示意 |

本地：`build_host.bat`（需本机 VS/CMake）。

## 板级文档（`skills/smart-car-dev/refs/`）

- `COMPILE_READY_WORKFLOW.md` — Skill→可编译代码  
- `SEEKFREE_BOARDS.md` / `BOARD_TEMPLATES.md` — 逐飞核心板  
- `CYT4BB7_DUAL_CORE.md` — 轮腿双核  
- `LS2K_LINUX_BOARD.md` — 走马龙芯  
- `RKNN_DEPLOY.md` — 智能模型 NPU  
- `IMU_INS_PLAYBOOK.md` / `ALGORITHMS_CODE_INDEX.md` — 惯导与算法索引  

使用指南副本：`skills/smart-car-dev/PERFECT_SKILL_GUIDE.md`

## 持续搜集 → 自评 → 选择性上传

本机工作区脚本：`_kb_raw/collect/`（见 `smart-car-dev/refs/CONTINUOUS_INGEST.md`）。

1. `collect_discover.py` 从 `discovery.json` 抽新候选  
2. `collect_evaluate.py` 出评估报告；**你**改 `decisions` 的 verdict  
3. `collect_apply.py` 只写入 accept 项的 INDEX/摘要并同步本仓 `skills/`  
4. 再 `git push`（仍不默认上传完整开源/`board_libs`）
