# 华侨大学_雁过留痕_PID调得非常队技术报告
- group: 本科·雁过留痕
- chars: 9102
- source: `华侨大学_雁过留痕_PID调得非常队技术报告.txt`

## 摘要片段
 第二十一届全国大学生智能汽车竞赛技术报告 学 校：华侨大学 队伍名称：PID调得非常队 参赛队员：邱锦浩 林剑腾 施淇元 带队老师：晏来成 张勇 关于技术报告和研究论文使用授权的说明 本人完全了解全国大学生智能汽车竞赛关保留、使用技术报告和研究论文的规定，即：参赛作品著作权归参赛者本人，比赛组委会可以在相关主页上收录并公开参赛作品的设计方案、技术报告以及参赛模型车的视频、图像资料，并将相关内容编纂收录在组委会出版论文集中。 参赛队员签名：邱锦浩 林剑腾 施淇元 带队教师签名： 晏来成 张勇 日 期： 2026.8.15 第二十一届全国大学生智能汽车竞赛 

## 技术信号
- **mcu**: STC32G144K246为主控, STC32G144K246主控, STC32G库开发, STC32G144K246, STC32G144K246的智能车系统设计, SYSTEM_CLOCK_96M
- **camera**: MT9V03X, 188×120, mt9v03x, 总钻风
- **control**: 增量式
- **vision**: 动态阈值
- **laser**: 激光
- **extra**: 负压, IMU660RA提供陀螺仪数据, 编码器, IMU660RA
- **element**: 斑马, 环岛, 十字, 障碍, 状态机

## 代码/符号锚点（报告中出现的标识符）
Get_err, Get_midline, Get_reference_point, MINMAX, Motor_contorl_handler, Search_line, Search_reference_col, barrier_analysis, barrier_handle, break_road, circle_handle, clock_init, cross_analysis, debug_init, element_count, element_search, encoder_speed_clear, encoder_speed_get, gpio_set_level, increment_pid_solve, key1_can, key3_can, main, motor_brake_control, motor_brake_reset, motor_brake_start, motor_control, my_abs, pit_all_init, pwm_set_duty, return, ring_target_detect, servo_pid_calculate, servo_select_params, speed_control, system_init, while, zebra_analysis

## 结构线索
- 日 期： 2026.8.15
- 第一章 引言 6
- 第二章 机械结构设计及布局 6
- 第三章 硬件电路设计与优化 9
- 第四章 软件设计及其优化 13
- 第五章 软件开发环境 17
- 第六章 车模主要参数说明 18
- 第七章 总结 19
- 第一章 引言
- 第二章 机械结构设计及布局
- 图2.1-2.3 车模俯视图、前视图、侧视图
- 图2.4 舵机安装与前桥转向机构
- 图2.5-2.6 摄像头、激光头、姿态传感器安装照片
- 图2.7 负压风扇安装与底盘加固照片
- 第三章 硬件电路设计与优化
- 图3.1 硬件电路整体架构框图
- 图3.2 主控板、扩展板或接口分配图
- 图3.3 电机驱动与负压风扇驱动电路
- 图3.4 激光头驱动电路示意图
- 第四章 软件设计及其优化
- 图4.1 主程序流程图
- 图4.3 圆靶识别与激光触发调试截图
- 第五章 软件开发环境
- 图5.1 上位机调试界面截图
- 第六章 车模主要参数说明

## 精读状态
- 扫描卡