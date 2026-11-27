# 山东大学_山青孤星_雁过留痕
- group: 本科·雁过留痕
- chars: 16928
- source: `山东大学_山青孤星_雁过留痕.txt`

## 摘要片段
第二十一届全国大学生智能汽车竞赛 技术报告 基于视觉与 IMU 辅助的智能车控制系统设计 学 校：山东大学（青岛） 队伍名称：山青孤星 参赛队员：蒋继宇、赵荣凯、晏陆君 带队教师：房明 竞赛组别：雁过留痕 完成日期：2026年08月18日 关于技术报告和研究论文使用授权的说明 本人完全了解全国大学生智能汽车竞赛关于保留、使用技术报告和研究论文的规定，即：参赛作品著作权归参赛者本人，比赛组委会可以在相关主页上收录并公开参赛作品的设计方案、技术报告以及参赛模型车的视频、图像资料，并将相关内容编辑收录在组委会出版论文集中。 参赛队员签名：____________________ 带队教师签名：____________________ 日期：________年____月____日 目录 第一章 引言 第二章 系统总体方案 第三章 机械结构与传感器安装 第四章 电路系统设计 第五章 控制软件设计 第六章 开发工具、制作安装与调试过程

## 技术信号
- **mcu**: STC32G144K246, STC32G144K, SYSTEM_CLOCK_96M, STC32G144K246_100PIN, STC32G144K_100PIN
- **camera**: MT9V03X, mt9v03x, 188×120
- **control**: 增量式, 差速, 阿克曼, 二次
- **vision**: 边线, 大津, 动态阈值
- **laser**: 激光, 打靶, 5 路激光
- **extra**: 编码器, IMU660RA, 负压, imu660ra_gyro_z, imu660ra_get_gyro, imu660_yaw_update_5ms, imu660ra, imu660
- **element**: 环岛, 斑马, 状态机

## 代码/符号锚点（报告中出现的标识符）
Attacking, Calc_curve_level, Encoder_GetValue, Error_sum, Fitted_Midline, Increment_PIDf, Island_analysis, Motor_Loop, OTSU_threshold, PID_servof, Ring, SERVO_PWM1, Search_line, Search_reference_col, Servo_Loop, Servo_pid_fit, Smart_Dream_speed, System_Init, Target_Accquire, binary_visual, clock_init, debug_init, func_abs, func_limit_ab, imu660_yaw_update_5ms, imu660ra_get_gyro, insert_val, ips200pro_display, main, mt9v03x_dma_handler, pit_handler, pit_ms_init, stop_check, while, wifi_display

## 结构线索
- 第一章 引言
- 第二章 系统总体方案
- 第三章 机械结构与传感器安装
- 第四章 电路系统设计
- 第五章 控制软件设计
- 第六章 开发工具、制作安装与调试过程
- 第七章 模型车主要技术参数
- 第八章 结论
- 第一章 引言
- 第二章 系统总体方案
- 第三章 机械结构与传感器安装
- 第四章 电路系统设计
- 本工程代码中主频配置为 96 MHz。开发环境使用逐飞 STC32G144K 开源库，库版本文件显示当前库版本为 V3.2.5。核心板说明书推荐使用 MDK FOR C251 V5.60，STC-ISP 下载软件需 V6.96F 以上。
- I_max = 1.8 Ah × 45 C = 81 A
- I_motor_no_load < 0.63 A × 2 = 1.26 A
- 主板摄像头接口包含 MT9V03X 摄像头接口、AL422B FIFO 和 SN74LVC1G00DCKR 逻辑器件。摄像头电源由 VCCCAM 提供，数据端口接入 AL422B，图像数据线 D0 到 D7 连接至主控 P2.0 到 P2.7。
- IMU 初始化后，程序每 5 ms 读取一次陀螺仪数据，并将 Z 轴角速度积分为航向角。积分时间步长为 0.005 s，角速度死区为 ±0.5 dps。
- 按照“只统计主板和电机驱动板”的口径，主板与电机驱动板合计可辨识电容容量约为 746.82 uF。
- KEY1 使用 P5.4，用于控制显示设备使能。
- KEY2 使用 P6.6，用于控制二值化显示。
- KEY3 使用 P6.7，用于激光全亮调试。
- 蜂鸣器使用 P6.5。
- 第五章 控制软件设计
- curve_level = 0.75 * curve_level_last + 0.25 * curve_level_raw
- servo_pidf.kp = curve_level * 0.5

## 精读状态
- 扫描卡