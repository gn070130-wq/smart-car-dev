# 湖北文理学院-雁过留痕-湖文孔明九队
- group: 本科·雁过留痕
- chars: 27249
- source: `湖北文理学院-雁过留痕-湖文孔明九队.txt`

## 摘要片段
Abstraction Layer） | | PIT | 周期中断定时器 | | PWM | 脉宽调制 | | PID | 比例-积分-微分控制器 | | UART | 通用异步收发器 | | SPI | 串行外设接口 | | IIC | 集成电路间总线 | | ToF | 飞行时间测距 | | `camera_value` | 视觉前瞻误差，代码中按 100 倍整数参与控制 | | `Island_State` | 环岛状态机状态变量 | | `Stop_flag` | 车辆运行/停止状态标志 | | `mod_flag` | 自动、参数或手动激光控制模式变量，具体模式命名待结合队伍协议补充 | --- # 第一章 引言 ## 1.1 研究背景 根据竞赛通知，参赛队伍需提交能够反映车模设计、制作和调试过程的技术报告，并同步准备车模检查材料。<sup>[1]</sup> 全国大学生智能汽车竞赛要求参赛队伍在限定车模、器

## 技术信号
- **mcu**: STC32G144K246, SYSTEM_CLOCK_99M, STC32G144K
- **camera**: MT9V03X, mt9v03x, 总钻风
- **control**: 模糊, 增量式, 阿克曼, 差速
- **vision**: 最长白列, 动态阈值, 差比和
- **laser**: 激光, 打靶, 预测, 7路激光
- **extra**: ToF, 编码器, DL1B, tof, IMU660RC, imu660rc, dl1b, IMU660_CONTROL, imu660rc_init
- **element**: 环岛, 状态机, 十字, 斑马, 坡道

## 代码/符号锚点（报告中出现的标识符）
Ackerman_diff, Cross_Detect, DuoJi_GetP, InitMH, LED_open, Longest_White_Column, PID_realize_encoder_L, PID_realize_encoder_R, Prospective_error, Steering_Servo_Control, Straight_Detect, TargetPredict_Control, TargetPredict_GetRoadPos, TargetPredict_GetRoadType, TargetPredict_Reset, TargetPredict_SelectLaser, Zebra_Found_stop, adc_mean_filter_convert, camera_pid_realize, camera_rx_interrupt_handler, clock_init, debug_init, difsum_left, dl1b_init, elif, get_encoder, huandao_mode, imu660rc_init, ips200_clear, ips200_displayimage03x, ips200_init, ips200_show, left, main, menu_set, mt9v03x_init, pit1_handler, show_line, sprintf, system_init

## 结构线索
- **表 0.1 报告基本信息**
- **图 0.1 技术报告及研究论文使用授权说明**
- **表 0.2 主要符号与缩略词**
- ## 1.1 研究背景
- ## 1.2 研究内容
- ## 1.3 报告结构
- 第一章说明研究背景与报告范围；
- 第二至四章介绍总体方案、机械结构和电路系统；
- 第五章重点说明软件架构、视觉算法与控制算法；
- 第六章总结开发调试过程；第七章给出技术参数和测试结果；
- 第八章总结工作并提出改进方向。
- ## 2.1 设计目标与约束
- ## 2.2 总体架构
- **图 2.1 系统总体架构图**
- ## 2.3 实时任务组织
- **图 2.2 实时事件协作时序图**
- ## 2.4 系统运行流程
- **图 2.3 主循环流程图**
- ## 3.1 车模总体结构
- **图 3.1 参赛车模总体结构**
- ## 3.2 传感器安装与坐标约定
- **图 3.2 七路激光阵列安装位置**
- **图 3.3 MT9V03X 摄像头安装结构**
- **图 3.4 DL1B ToF 测距模块安装位置**
- **图 3.5 IMU660RC 安装位置**

## 精读状态
- 精读完成（主锚点）