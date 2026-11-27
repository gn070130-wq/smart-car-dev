# 山东师范大学_山师雁过_雁过留痕
- group: 本科·雁过留痕
- chars: 17265
- source: `山东师范大学_山师雁过_雁过留痕.txt`

## 摘要片段
第二十一届全国大学生智能汽车竞赛 技术报告 学 校：山东师范大学 队伍名称：山师燕过 参赛队员：郝朝通 梁先汇 司鹏翼 带队老师： 许化强 周茂霞 关于技术报告以及研究论文使用授权的说明 本人（队）自愿参加第二十一届全国大学生智能汽车竞赛，同意全国大学生智能汽车竞赛组委会关于参赛作品使用授权的规定，即：参赛作品著作权归参赛者本人，比赛组委会可以在相关主页上收录并公开参赛作品的设计方案、技术报告以及参赛模型车的视频、图像资料，并将相关内容编纂收录在组委会出版论文集中。 参赛队员签名：＿＿＿＿＿＿ 带队教师签名：＿＿＿＿＿＿ 日 期：＿2026.8.19＿ 目 录 第一章 引言 1 第二章 系统总体方案设计 2 2.1 模型车设计制作的主要思路 2 2.2 实现的技术方案概要 2 第三章 机械结构设计与安装 3 3.1 车模机械部分安装及改造 3 3.2 传感器的设计与安装 

## 技术信号
- **mcu**: STC32G144K246为核心的电路设计, STC32G144K246单片机为主控核心, STC32G144K246一片, STC32F12K54一片, STC32G12K128两片, STC32G144K246负责摄像头图像处理, STC32单片机开发, 主频96MHz, STC32单片机为主控, SYSTEM_CLOCK_96M
- **camera**: MT9V03X
- **control**: 差速, 二次, 阿克曼, 位置式
- **vision**: 边线, 动态阈值, 八邻域, 差比和, 逆透视
- **laser**: 激光, 打靶, 云台, 靶环, 预测
- **extra**: 负压, IMU660RA用于感知车模的俯仰与横滚姿态, ToF, 编码器, IMU660RA与正交编码器, IMU660RA经P21接口与主控相连, imu660ra_get_gyro, imu660ra_gyro_y, imu660ra_gyro_z, IMU660RA
- **element**: 十字, 环岛, 坡道, 斑马, 状态机

## 代码/符号锚点（报告中出现的标识符）
Fan_Vac_Reset, Fan_Vac_Update, Menu_Input_1ms, Odom_Fusion_Step, Servo_Control_Update, Target_Odom_Propagate_1ms, Target_Ptz_Tick_1ms, Zebra_Stop_Tick_1ms, adc_mean_filter_convert, bat_raw_to_cv, bat_voltage_sample, bat_voltage_startup_calibrate, clock_init, crc8_2, debug_init, encoder_clear_count, encoder_get_count, imu660ra_get_gyro, imu_pit_handler, l_duty_set, l_motor_pid, main, pwm_set_duty, r_duty_set, r_motor_pid, return, servo_pit_handler, uart_send_pit_handler, uart_write_buffer_noblock

## 结构线索
- 日 期：＿2026.8.19＿
- 第一章 引言 1
- 第二章 系统总体方案设计 2
- 第三章 机械结构设计与安装 3
- 第四章 电路设计说明 5
- 第五章 控制软件设计 6
- 第六章 系统开发与调试过程 8
- 第七章 模型车主要技术参数 10
- 第八章 结论 11
- 第一章 引言
- 第二章 系统总体方案设计
- 第三章 机械结构设计与安装
- 第四章 电路设计说明
- 图4-4 3.3V LDO电源电路
- 5. 5V外设电源控制。5V外设电源（VCC5V_EXT）采用限流负载开关SY6280AAAC进行控制，可对激光头、ToF测距模块等外设的供电进行通断管理，并具备过流保护功能，限流值由ISET引脚外接的4.7kΩ电阻设定，LED1为该路电源的指示灯。电路如图4-5所示。
- 第五章 控制软件设计
- 第六章 系统开发与调试过程
- 软件环境方面，在电脑上安装Keil建立工程并配置好编译选项，配合VS Code进行代码编辑，通过烧录器连接主控板的调试口，实现程序下载和在线调试。6.3 调试过程
- 第七章 模型车主要技术参数
- 模型车的主要技术参数如表7.1所示。
- 表7.1 模型车主要技术参数
- 第八章 结论
- 改造后车模总体重量（含电池） | 1.4kg

## 精读状态
- 扫描卡