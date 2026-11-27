# 单车定向 · 检索索引

说编号即可展开，例如：`展开 D3`、`展开 B7`、`写平衡环伪代码`、`对比 PurePursuit vs Stanley`。

## D. 模块索引

| ID | 模块 | 要点 |
|----|------|------|
| D1 | 组别与规则 | N1、室外、八字/绕锥/坡草；禁负压 |
| D2 | 硬件栈 | TC264 + FOC + 舵机 + IMU + 磁编 |
| D3 | 软件分层/文件 | 见 FILE_STRUCTURE.md |
| D4 | 姿态/惯导 | EKF、零偏、INS 推算 |
| D5 | 寻迹族 | 航向回放 / PP / Stanley / 杜宾斯 |
| D6 | 平衡与速度 | 角环+陀螺环+增量速度 PID |
| D7 | 状态机与保护 | 科目切换、安全距离、倾角保护 |
| D8 | 调试流程 | 静态→架空→低速平衡→采样→回放 |
| D9 | 竞赛风险 | 见 COMPARE_AND_OPT.md |

## B. 算法卡片（详见 ALGORITHMS.md）

| ID | 算法 |
|----|------|
| B1 | IMU 零偏 + 四元数 EKF |
| B2 | 航位推算 INS |
| B3 | 轨迹采样/Flash 回放 |
| B4 | 逐点导航方位误差 |
| B5 | IMU_Go 航向回放 |
| B6 | Pure Pursuit + 曲率前馈 |
| B7 | Stanley |
| B8 | 杜宾斯绕锥 |
| B9 | 横滚平衡串级 PID |
| B10 | 增量式速度 PID |
| B11 | UART FOC 协议帧 |
| B12 | 余弦绕锥 Task_Cone |

## 精读锚点队伍

1. **中北破晓**（文件树+附录源码最完整）  
2. **杭电单车定向一队**（杜宾斯+半开环速度+惯导）  
3. **东南饿饿饿灵骑士**（Stanley+ADRC+QEKF）  
4. **中国海大闪电龟龟**（三闭环+参数插值调度）  
5. **合工大/哈工大深圳/厦理** 等作对照  

## 路径

| 路径 | 内容 |
|------|------|
| `LEARNING_SUMMARY.md` | 学习摘要 |
| `INDEX.md` | 本索引 |
| `FILE_STRUCTURE.md` | 工程文件说明 |
| `ALGORITHMS.md` | 算法拆解（含可写代码模板） |
| `COMPARE_AND_OPT.md` | 对比/短板/优化/竞赛评估 |
| `corpus/` | 47 份报告正文 |
| `MINE_SIGNALS.md` | 关键词统计 |
| `DEEP_NOTES.md` | 长文摘录 |
