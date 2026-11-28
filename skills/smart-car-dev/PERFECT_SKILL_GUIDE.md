# 智能车软件 · Skill 使用指南

权威仓：https://github.com/gn070130-wq/smart-car-dev  
目录总表：`skills/README.md` · 根入口：仓库根 `SKILL.md`

## 你现在拥有什么

- **竞速十一组** + **创意五组**（各有 `SKILL` / SCAFFOLD / RECOMMENDED / refs）
- 总控 `smart-car-dev`：COMPILE_READY、板级、算法索引、惯导
- 分流 `smartcar-competition` + ML `smartcar-ml-optimize`
- 可编译示例：仓库根 `generated/`（PVC + 分板 stub）
- 走马专题：仓库根 `references/`（报告语料与多仓解析）

## 对 Agent 怎么说

```text
用 smartcar-competition 定组，再用「某组」skill 按 SCAFFOLD 搭软件。

用 smart-car-dev，按 COMPILE_READY_WORKFLOW：组别…… 输出到 generated/xxx 并说明如何编译。

用疯狂电路 skill，对照 RECOMMENDED 改电磁中线；上板读 BOARD_TEMPLATES。

轮腿上 CYT4BB7：先读 CYT4BB7_DUAL_CORE；走马读 LS2K_LINUX_BOARD；模型读 RKNN_DEPLOY。
```

## 竞速十一组

| 赛道 | Skill | 常见板 |
|------|-------|--------|
| 飞檐走壁 | `feiyan-zoubi` | STC32 |
| 疯狂电路 | `fengkuang-dianlu` | TC264/364 |
| 蚂蚁搬家 | `mayi-banjia` | 视规则 |
| 飞跃雷区 | `feiyue-leiqu` | 飞控相关 |
| 走马观碑 | `zouma-guanbei` | LS2K/Linux |
| 雁过留痕 | `yanguo-liuhen` | STC32G144 |
| 智能视觉 | `zhineng-shijue` | RT1064 |
| 智能模型 | `zhineng-moxing` | RK3588+下位机 |
| 卡丁快跑 | `kading-kuaipao` | TC387 |
| 轮腿穿越 | `lunhui-chuanyue` | CYT4BB7 |
| 单车定向 | `danche-dingxiang` | TC264+FOC |

## 创意组

| 赛道 | Skill |
|------|-------|
| 智慧医疗 | `zhihui-yiliao` |
| 智慧工厂 | `zhihui-gongchang` |
| 智慧救援 | `zhihui-jiuyuan` |
| 智慧城市 | `zhihui-chengshi` |
| 智慧农场 | `zhihui-nongchang` |

## 板级与示例

| 文档/工程 | 用途 |
|-----------|------|
| `BOARD_TEMPLATES` / `SEEKFREE_BOARDS` | 逐飞上板 |
| `CYT4BB7_DUAL_CORE` | 双核 |
| `LS2K_LINUX_BOARD` / `RKNN_DEPLOY` | 龙芯 / NPU |
| `IMU_INS_PLAYBOOK` | 惯导平衡 |
| `generated/board_*_stub`、`pvc_line_track` | host 可编译壳 |
| 本机 `board_libs/` | 逐飞官方库全文（Git 仅 INDEX） |

## 强制调用顺序（赛道）

`SCAFFOLD` → `RECOMMENDED` → `ANALYSIS` → `refs/FULL_SUMMARY` + `ALGORITHMS` →（需要时）ml-optimize / COMPILE_READY

## 持续搜集（可选）

见 `refs/CONTINUOUS_INGEST.md`：`collect_discover` → `collect_evaluate`（你改 decisions）→ `collect_apply` → push GitHub。  
默认不上大体积开源树；只选择性同步元数据/摘要。
