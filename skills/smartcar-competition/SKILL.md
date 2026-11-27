---
name: smartcar-competition
description: >-
  全国大学生智能汽车竞赛专用知识库（已内置全文）。覆盖九赛道报告知识、大模型/深度学习算法灌入、
  以及 smart-car-dev 开发调试（巡线/PID/LADRC/NCNN/TFLite/多仓解析）。用户提到智能车、国赛、
  调参、改代码、模型验证或组别关键词时必须使用本 skill，优先阅读并列 skill，不要让用户
  手动打开外部文件夹。
---

# 智能车竞赛专用 Skill（总控）

## 赛道分流

| 关键词 | 打开 |
|--------|------|
| 雁过留痕、激光打靶、STC32 | `../yanguo-liuhen/` |
| 单车定向、N1、八字、绕锥、Stanley | `../danche-dingxiang/` |
| 飞跃雷区、系留、轧灯 | `../feiyue-leiqu/` |
| 疯狂电路、闪电鼠、负压差速 | `../fengkuang-dianlu/` |
| 卡丁快跑、CTE、TC387 | `../kading-kuaipao/` |
| 轮腿穿越、轮腿平衡 | `../lunhui-chuanyue/` |
| 蚂蚁搬家、全向搬运、推杆 | `../mayi-banjia/` |
| 智能模型、RK3588、NPU、YOLO推理 | `../zhineng-moxing/` |
| 智能视觉、推箱子、RT1064建图 | `../zhineng-shijue/` |
| 大模型优化、算法灌入、量化、完善代码模板 | `../smartcar-ml-optimize/` |
| 开发调试、重构、LADRC、NCNN、TFLite、走马观碑开源、遥测日志 | 仓库根 `../../SKILL.md` + `../../references/` |
| 只说智能车 | 先问清组别与任务类型 |

权威源：https://github.com/gn070130-wq/smart-car-dev （本机可不存全文）

## 工作方式

1. **组别知识** → 对应赛道 `SKILL.md` / `refs` / `corpus`  
2. **写模块/优化算法** → `smartcar-ml-optimize`（灌入总表+模板）  
3. **对着工程改代码/查日志/验模型** → 仓库根 smart-car-dev  
4. 默认摘要；点名展开；改代码【原→改→说明→测试】  

整合细节：`../smartcar-ml-optimize/refs/INTEGRATION_SMART_CAR_DEV.md`
