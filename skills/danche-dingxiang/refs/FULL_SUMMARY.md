# 单车定向 · 报告级知识摘要

资料：参赛技术报告语料（约 47 份）+ 本机开源仓。详解见 `LEARNING_SUMMARY.md`、`ALGORITHMS.md`。

## 赛题要点

- 车模：**N1 两轮单车**（前轮舵机 + 后轮无刷）；禁负压/加宽胎等辅助平衡  
- 室外科目：往返直线（或取消）、**慢速八字**、草地/坡道/**锥桶绕桩**  
- 主矛盾：**自平衡 + 短时惯导寻迹**（摄像头非标配）

## 默认软件栈

```text
IMU 零偏 → 四元数 EKF(Roll/Pitch/Yaw)
编码器/驱动板速度 → 航位推算(INS_X,Y)
轨迹 Flash 回放 / Stanley / Pure Pursuit / 杜宾斯
任务 FSM：直线 / 八字 / 绕锥 / 坡草 / 停车
【最高优先】横滚角+角速度串级 → 舵机
速度增量 PID → UART→FOC
倾角/偏航/电压保护 → 降功率停车
```

算法锚点：`P02` + `C04/C06` 平衡 + `C02/C07` Stanley/PP + `S01` + `S02`

## 硬件共识

| 模块 | 常见 |
|------|------|
| 主控 | TC264（ADS）；少数 TC377 |
| FOC | CYT2BL3 / 自制，UART 收速度 |
| 电机/舵机 | BDS3620 + BDS300 |
| 姿态 | IMU660RA 等六轴 |
| 测速 | TLE5012 / 正交编码器 |

## 安全

- 倾角超限停转；偏航越界降功率  
- 上电舵机中位、电机 0  
- 遥控急停优先于寻迹  

## 对照

- `code_opensource/RECOMMENDED.md`（motorcycle / ZJX_21_Bike 等）  
- 总控：`../../smart-car-dev/refs/IMU_INS_PLAYBOOK.md`、`STANLEY_PURE_PURSUIT.md`  
