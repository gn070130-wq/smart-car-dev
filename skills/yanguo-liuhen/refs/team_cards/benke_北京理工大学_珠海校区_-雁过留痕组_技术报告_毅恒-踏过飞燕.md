# 北京理工大学_珠海校区_-雁过留痕组_技术报告_毅恒-踏过飞燕
- group: 本科·雁过留痕
- chars: 102407
- source: `北京理工大学_珠海校区_-雁过留痕组_技术报告_毅恒-踏过飞燕.txt`

## 摘要片段
 基于STC32G144K246的智能车系统设计 ——第二十一届全国大学生智能汽车竞赛雁过留痕组技术报告 参赛学校：北京理工大学（珠海） 队伍名称：毅恒-踏过飞燕 参赛队员：林浩康 杨衡锋 潘振东 指导教师：王庆娟 方宇杰 竞赛组别：雁过留痕组 使用授权书 本人授权第二十一届全国大学生智能汽车竞赛组委会可以将本技术报告（包括附录中的源代码）用于竞赛评审、教学交流、论文集编撰等非商业用途。 授权人（参赛队员签字）： 林浩康 ________________ 杨衡锋 ________________ 潘振东 ________________ 指导教师签字： 王庆娟 ________________ 方宇杰 ________________ 2026年 月 日 目 录 第一章 引言 1.1 竞赛背景与组别任务 1.2 相关技

## 技术信号
- **mcu**: STC32G144K246的智能车系统设计, STC32G144K246, STC32G144K246单片机, STC32G144K246芯片规格书与用户手册, STC32G144K246单片机为核心控制器, STC32G144主板的编码器接口, STC32G144主板通过铜柱固定于车模_后轮上方__位置, STC32G144K246是STC公司推出的高性能32位8051内核单片机, STC32G144K246的内存和算力限制, STC32G144K246单片机和神眼摄像头
- **camera**: 总钻风, 神眼, 188×120, mt9v03x, MT9V03X
- **control**: 增量式, 位置式
- **vision**: 大津, 八邻域, 最长白列, 边线
- **laser**: 激光, 云台, 靶环, 打靶
- **extra**: 编码器, 涵道, 负压, imu660ra_gyro_z, imu660ra_get_gyro, imu660ra_gyro_transition
- **element**: 十字, 环岛, 坡道, 斑马, 状态机

## 代码/符号锚点（报告中出现的标识符）
All_PID_control, All_init, IMU_data_get, RBG_COLOR, RGB_strategy, Speed_strategy, TIM0_PIT_5ms, TIM1_PIT_1ms, beep_start, calculate_centerline_error, check_stop_line, compress_gray_image, draw_thick_line, element_circle_left, element_circle_right, element_crossroad, element_estimate_cross, element_estimate_left_circle, element_estimate_right_circle, element_estimate_target, element_target, encoder_get, find_tracking_line, get_MAX_white_column, get_left, get_right, get_start_point, get_threshold_value, gpio_get_level, gpio_set_level, image_capture, imu660ra_get_gyro, imu660ra_gyro_transition, imu_all_clear, increment_PID, judge_big_small_circle, judge_target, key_scan, limit_val, location_PID

## 结构线索
- 第一章 引言
- 1.1 竞赛背景与组别任务
- 1.2 相关技术文献综述
- 1.3 报告框架
- 第二章 车模整体设计
- 2.1 系统概述
- 2.2 整车布局及参数
- 第三章 机械系统设计
- 3.1 车模结构与改造
- 3.2 编码器安装
- 3.3 舵机与激光云台安装
- 3.4 电池与电路板固定
- 第四章 硬件设计及实现
- 4.1 主控MCU（STC32G144K246）
- 4.2 总钻风摄像头模块
- 4.3 DRV8701E电机驱动电路
- 4.4 编码器接口电路
- 4.5 激光模块
- 4.6 电源管理电路
- 4.7 其他外设电路
- 第五章 软件设计及算法
- 5.1 开发环境与软件架构
- 5.2 图像采集与预处理
- 5.3 大津法二值化与帧率优化
- 5.4 赛道搜线算法

## 精读状态
- 深扫（长文已入卡，可再点名展开）