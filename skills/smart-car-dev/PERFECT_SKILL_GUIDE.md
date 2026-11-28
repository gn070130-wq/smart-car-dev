# 智能车软件 · 完美 Skill 使用指南

## 你现在拥有什么

- **竞速十一组**本地开源（约 120 仓）+ SCAFFOLD / RECOMMENDED / ANALYSIS
- **创意组** skill（智慧医疗等，约 12 仓）
- 报告知识 `refs/` / `corpus/`（多数竞速组已挂接）
- 算法模板 `smartcar-ml-optimize`
- 调试与骨架 `smart-car-dev/refs/`（含 `SOFTWARE_SKELETON.md`）

## 对 Agent 怎么说（示例）

- 「用竞速组疯狂电路 skill，按 SCAFFOLD 搭控制软件」
- 「用创意组智慧医疗 skill，对照推荐仓改 ROS 节点」
- 「对照推荐仓改转向环，原→改→说明→测试」

## 竞速十一组

| 赛道 | Skill | 本地仓约 |
|------|-------|----------|
| 飞檐走壁 | `feiyan-zoubi` | 22 |
| 疯狂电路 | `fengkuang-dianlu` | 18 |
| 蚂蚁搬家 | `mayi-banjia` | 17 |
| 飞跃雷区 | `feiyue-leiqu` | 20 |
| 走马观碑 | `zouma-guanbei` | 7 |
| 雁过留痕 | `yanguo-liuhen` | 13 |
| 智能视觉 | `zhineng-shijue` | 5 |
| 智能模型 | `zhineng-moxing` | 3 |
| 卡丁快跑 | `kading-kuaipao` | 7 |
| 轮腿穿越 | `lunhui-chuanyue` | 5 |
| 单车定向 | `danche-dingxiang` | 6 |

## 创意组

| 赛道 | Skill | 本地仓约 |
|------|-------|----------|
| 智慧医疗 | `zhihui-yiliao` | 9 |
| 智慧工厂 | `zhihui-gongchang` | 1 |
| 智慧救援 | `zhihui-jiuyuan` | 1 |
| 智慧城市 | `zhihui-chengshi` | 1 |
| 智慧农场 | `zhihui-nongchang` | 1（稀疏，无 datasets） |

补充能力：
- `refs/BOARD_TEMPLATES.md` / `SEEKFREE_BOARDS.md` / `CYT4BB7_DUAL_CORE.md`
- 本机逐飞库：`board_libs/`（CYT4BB7、TC264/364/377/387、RT1064、STC32、CYT2BL3）
- 示例：`generated/board_seekfree_stub/`、`board_cyt4bb7_stub/`、`pvc_line_track/`
- 弱组报告级见各 skill `refs/FULL_SUMMARY.md`。

分流总控：`smartcar-competition`
