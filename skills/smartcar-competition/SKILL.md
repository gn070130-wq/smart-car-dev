---
name: smartcar-competition
description: >-
  国赛竞速十一组 + 创意组分流。用户未指定组别、只说智能车/竞赛，或同时要
  「推荐算法 + 代码结构」时先用本 skill 定组，再交给 smart-car-dev 的
  QUESTION_TO_ARCHITECTURE 流程与对应赛道 skill。
---

# 竞赛总控（竞速 + 创意）

## 提问类需求

若用户要 **推荐算法 / 代码结构 / 怎么搭软件**：

1. 用下表锁定组别（不明则先问一句）  
2. 打开 `../smart-car-dev/refs/QUESTION_TO_ARCHITECTURE.md`  
3. 再读该赛道 `RECOMMENDED.md` + `smartcar-ml-optimize` 算法表  

## 竞速十一组

| 关键词 | Skill |
|--------|-------|
| 飞檐走壁、爬墙 | `feiyan-zoubi` |
| 疯狂电路、电感、TC264 | `fengkuang-dianlu` |
| 蚂蚁搬家、搬运 | `mayi-banjia` |
| 飞跃雷区、飞控 | `feiyue-leiqu` |
| 走马观碑、龙芯 | `zouma-guanbei` |
| 雁过留痕、激光 | `yanguo-liuhen` |
| 智能视觉、RT1064 | `zhineng-shijue` |
| 智能模型、NPU | `zhineng-moxing` |
| 卡丁快跑、TC387 | `kading-kuaipao` |
| 轮腿穿越、平衡腿 | `lunhui-chuanyue` |
| 单车定向、绕锥 | `danche-dingxiang` |

## 创意组

| 关键词 | Skill |
|--------|-------|
| 智慧医疗 | `zhihui-yiliao` |
| 智慧工厂 | `zhihui-gongchang` |
| 智慧救援 | `zhihui-jiuyuan` |
| 智慧城市 | `zhihui-chengshi` |
| 智慧农场 | `zhihui-nongchang` |

算法总表：`../smartcar-ml-optimize/refs/INGESTED_ALGORITHMS.md`  
结构骨架：`../smart-car-dev/refs/SOFTWARE_SKELETON.md`  
可编译流程：`../smart-car-dev/refs/COMPILE_READY_WORKFLOW.md`  
板级：`../smart-car-dev/refs/BOARD_TEMPLATES.md` · 使用指南：`../smart-car-dev/PERFECT_SKILL_GUIDE.md`
