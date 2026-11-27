# 项目文件结构说明（主锚：中北·破晓 / 逐飞 TC264 体系）

> 本包无独立工程压缩文件；以下为多队报告共识 + 破晓附录源码路径。你的实车工程若不同，用同名职责对齐即可。

## 典型双工程拆分

```
[主控工程 · ADS / TC264]
  IMU/Attitude.c(.h)      # 零偏、四元数EKF、姿态输出
  imu.c / Navigation.c    # 轨迹记录回放、点距、方位、Pure Pursuit
  Stanley.c               # Stanley 横向控制（可选）
  balance.c               # 平衡角环/陀螺环/转向环/速度环 PID
  car_control.c           # 科目模式、状态机、Car_control()
  small_driver_uart_*.c   # 与 FOC 板 UART 协议
  main / 周期中断         # 调度 IMU→导航→控制→通信

[驱动工程 · IAR / CYT2BL3]
  FOC / 电流环 / 速度环
  磁编 TLE5012 角度
  UART 收占空比、回传速度
```

## 入口与配置

| 项 | 说明 |
|----|------|
| 入口 | 主控 `main`：时钟/外设 init → `Attitude_Init` → 循环/`PIT` 调 `Car_control` |
| 核心配置 | 控制周期（IMU/平衡优先）、舵机中位与限幅、速度限幅、点距 `imu_point_step_cm`、PP 参数表、UART 波特率（如 460800） |
| Flash | `Write_IMU` / `Write_IMU_XY` / `Flash_Init` 存 yaw 与轨迹点 |

## 模块依赖（简图）

```
Attitude(IMU) ──► QEKF_INS.Yaw/Roll
驱动板速度 ──► INS_Speed_Calc ──► INS_X/Y
Flash轨迹 ──► IMU_Go / PurePursuit / Stanley ──► delt_degree / servo_pid_err
car_control ──► balance_*_PID ──► Servo PWM + UART duty
Path_Safety_Check / 倾角保护 ──► stop_flag
```

## 杭电变体差异

- 科目分策略 + UI；锥桶用 **杜宾斯曲线**  
- 速度「半开环」：PWM-转速表 + 反转急减速  
- 电调与主板快拆母子板；开发环境同为 ADS  

## 与雁过留痕工程对比（防混淆）

| | 单车定向 | 雁过留痕 |
|--|----------|----------|
| 主控 | 多为 TC264 | 多为 STC32 |
| 主感知 | IMU+里程计 | 摄像头边线 |
| 控制核心 | 平衡串级 | 循迹 PD/纯跟踪 |
| 特色任务 | 八字/绕锥室外 | 激光打靶室内 |
