# 成都大学_十字线_雁过留痕
- group: 本科·雁过留痕
- chars: 11758
- source: `成都大学_十字线_雁过留痕.txt`

## 摘要片段
摘 要 本文设计的智能车系统以 STC32G144K246核心控制单元，使用 Keil 5软件开发平台进行设计。通过车体前方的总钻风灰度摄像头传感器采集二值化后提取赛道信息；通过正交编码器检测模型车的实时速度，使用了PID控制算法来实现转速控制，使用阿克曼模型控制后轮差速，实现了对车运动速度和运动方向的闭环控制。并且为了提高模型车的速度和稳定性，使用Matlab、图像上位机、无线图传模块、IPS屏以及按钮等调试工具，进行了大量硬件与软件测试。实验结果表明，该系统设计方案稳定可行。 关键字：智能车，STC32G144K246，总钻风灰度摄像头，图像处理算法，电机PID控制 引 言 全国大学生智能汽车竞赛是以“立足培养、重在参与、鼓励探索、追求卓越”为指导思想，鼓励创新的一项创意性科技竞赛，是面向全国大学生的一种具有探索性工程实践活动。组委会提供汽车标准模型以及电机，使用指定单片机为核心控制模块，并通过使用道路传感器对黑

## 技术信号
- **mcu**: STC32G144K246核心控制单元, STC32G144K246, STC32G144K246主控芯片, STC32G144K246内存的不足, STC32G144K246作为核心控制单元用于智能车系统的控制, STC32G144K246作为差速实时转向和速度控制的依据, STC32G的开源方案, STC32G系列, SYSTEM_CLOCK_96M
- **camera**: 总钻风, 188×120, mt9v03x
- **control**: 阿克曼, 差速, 模糊, 模糊pid, 增量式, 滑模, 位置式
- **vision**: 差比和, 边线, 动态阈值, 大津
- **laser**: 激光, 打靶, 预测
- **extra**: 编码器
- **element**: 十字, 障碍, 环岛, 坡道

## 代码/符号锚点（报告中出现的标识符）
BM_Control, Circle_Angle, Device_Init, Direction_PID_Init, Flash_Control, GetMotorPulse, Get_BM_Status, Image_Deal, Image_PID_Calculate, LIMIT, Laser_Close, Motor_Test, My_Debug, My_LPF_1, Outside_Protect, PID_Init, Proccess_IMU, Run_Control, Speed_PID_Calculate, Speed_PID_Init, clock_init, debug_init, gpio_set_level, interrupt_set_priority, main, pit_handler, pit_ms_init, seekfree_assistant_camera_send, while

## 结构线索
- 日期：2026.8.15
- 第一章 系统总体设计
- 第二章 机械结构调整及优化
- 图2.1、图2.2 车轮转向结构视图
- 图 2.4 编码器安装
- 图 2.5 齿轮啮合
- 第三章 硬件电路方案设计
- 图3.1 主控核心
- 图3.2 外设接口
- 本系统主要设计了舵机部分供电（7.3V），主控核心供电（3.3V），屏幕供电（3.3V）以及电调系统供电（12V）。
- 图3.4 电源电路
- 舵机供电采用的电源方案是TPS565208DDCR输出7.3V。
- 图3.5 舵机供电
- 图3.6 5V系统供电
- 5V再通过3颗RT9013降压至3.3V分别给MCU，摄像头以及其他外设供电
- 图3.7 MCU、摄像头、其他外设供电
- 图3.9 电调电路
- 主板正反面PCB如图3.10：
- 图3.10主板PCB正反面
- 电调板正反面PCB如图3.11：
- 图3.11 电调板PCB正反面
- 激光驱动板正反面PCB如图3.13：
- 图3.13 电调板PCB正反面
- 第四章 软件算法系统设计
- 4.1.1程序整体运行流程图

## 精读状态
- 扫描卡