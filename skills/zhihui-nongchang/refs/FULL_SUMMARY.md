# 智慧农场（创意）· 知识摘要

流程不变。公开整车代码极少。

## 本机对照

- 稀疏入库：`code_opensource/21届_智慧农场__github__indulgeuuu-del__smart_farm_models`（脚本/配置/部署壳，**无** datasets）  
- 车控仍用 `SOFTWARE_SKELETON` + 任务 FSM  

## 软件链路

```text
作物/路径感知(V) → 任务 S01（巡垄/作业点）
→ 底盘 C05 → S03
模型训练/导出可参考该仓 scripts/configs/deploy
```

注意 Windows 勿全量 clone datasets（路径过长）；需要数据时在上游按文档单独取。
