# 智能车软件 · Skill 使用指南

## 你现在拥有什么

- **竞速十一组** + **创意五组**（`skills/` 下各目录）
- 每组：`SKILL` → `code_opensource/{SCAFFOLD,RECOMMENDED,INDEX,ANALYSIS}` → `refs/FULL_SUMMARY` + `ALGORITHMS`
- 总控 `smart-car-dev`：可编译流程、板级文档、算法索引
- ML：`smartcar-ml-optimize`（算法 ID / 模板 / 量化）
- 可编译示例：仓库根 `generated/`（PVC + 分板 stub）
- 走马专题与多仓解析：仓库根 `references/`（与根 `SKILL.md` 联用）

## 对 Agent 怎么说

- 「用 smartcar-competition 定组，再按该组 SCAFFOLD 搭软件」
- 「用 smart-car-dev，按 COMPILE_READY 输出到 generated/xxx 并本地编译」
- 「用疯狂电路 skill，对照 RECOMMENDED 改电磁中线」
- 「上 CYT4BB7 / TC264 / STC32 / RT1064 / LS2K / RKNN 时先读对应 refs」

## 竞速十一组

| 赛道 | Skill |
|------|-------|
| 飞檐走壁 | `feiyan-zoubi` |
| 疯狂电路 | `fengkuang-dianlu` |
| 蚂蚁搬家 | `mayi-banjia` |
| 飞跃雷区 | `feiyue-leiqu` |
| 走马观碑 | `zouma-guanbei` |
| 雁过留痕 | `yanguo-liuhen` |
| 智能视觉 | `zhineng-shijue` |
| 智能模型 | `zhineng-moxing` |
| 卡丁快跑 | `kading-kuaipao` |
| 轮腿穿越 | `lunhui-chuanyue` |
| 单车定向 | `danche-dingxiang` |

## 创意组

| 赛道 | Skill |
|------|-------|
| 智慧医疗 | `zhihui-yiliao` |
| 智慧工厂 | `zhihui-gongchang` |
| 智慧救援 | `zhihui-jiuyuan` |
| 智慧城市 | `zhihui-chengshi` |
| 智慧农场 | `zhihui-nongchang` |

## 板级与示例

- 文档：`BOARD_TEMPLATES` / `SEEKFREE_BOARDS` / `CYT4BB7_DUAL_CORE` / `LS2K_LINUX_BOARD` / `RKNN_DEPLOY` / `IMU_INS_PLAYBOOK`
- 示例：`generated/board_seekfree_stub`、`board_tc264_stub`、`board_stc32_stub`、`board_rt1064_stub`、`board_cyt4bb7_stub`、`pvc_line_track`
- 逐飞库全文：本机 `board_libs/`（Git 仅 INDEX）

分流：`smartcar-competition` · 目录说明：`skills/README.md`
