# 天津科技大学_疾风六队_雁过留痕
- group: 本科·雁过留痕
- chars: 12014
- source: `天津科技大学_疾风六队_雁过留痕.txt`

## 摘要片段
第二十一届全国大学生智能汽车竞赛 技 术 报 告 参赛学校：天津科技大学 队伍名称：疾风六队 竞赛组别：雁过留痕 参赛队员：徐永昊、夏志强、陈瑜灿 带队老师：段英宏、刘秀红 2026 年 8 月 关于技术报告和研究论文使用授权的说明 本人完全了解全国大学生智能汽车竞赛关于保留、使用技术报告和研究论文的规定，即：参赛作品著作权归参赛者本人，比赛组委会可以在相关主页上收录并公开参赛作品的设计方案、技术报告以及参赛模型车的视频、图像资料，并将相关内容编纂收录在组委会出版论文集中。 参赛队员姓名：徐永昊、夏志强、陈瑜灿 带队教师签名：段英宏、刘秀红 日期：2026年8月18日 目 录 关于技术报告和研究论文使用授权的说明 II 目 录 1 第一章 引 言 2 1.1 竞赛介绍 2 1.2 第二十一届智能车大赛规则介绍 3 1.3 车模总体方案概述 3 第二章 机械结构设计 3 2.1 车模选择与整体布局 3 2.2 传感器

## 技术信号
- **mcu**: STC32G144K, 主频 114MHz, STC32G进行无刷电机的三路控制, SYSTEM_CLOCK_114M
- **camera**: MT9V03X, 总钻风, 188x120, 80×60
- **control**: 差速, 阿克曼, 增量式
- **vision**: 动态阈值, 逆透视
- **laser**: 激光, 打靶, 靶环
- **extra**: IMU660RA, 编码器, imu660ra_get_gyro, imu660ra_gyro_z
- **element**: 十字, 环岛, 坡道, 状态机, 斑马

## 代码/符号锚点（报告中出现的标识符）
buzzer_init, clock_init, debug_init, encoder_clear_count, encoder_get_count, flash_init, gpio_set_level, image_init, image_update, image_update_laser_test, imu660ra_get_gyro, main, motor_init, motor_limit, motor_output, motor_start_control, motor_timer, motor_update_fan, para_init, pwm_set_duty, servo_init, servo_limit, servo_update, servo_update_motor_target, state_init, switch, system_delay_ms, ui_init, ui_is_debug, ui_update, while

## 结构线索
- 第一章 引 言 2
- 第二章 机械结构设计 3
- 第三章 硬件电路的设计 9
- 第四章 总体理论分析及控制软件算法实现 27
- 第五章 开发工具、制作、安装、调试过程说明 30
- 第六章 车模的各项参数 30
- 第一章 引 言
- 第二章 机械结构设计
- 第三章 硬件电路的设计
- 第四章 总体理论分析及控制软件算法实现
- 第五章 开发工具、制作、安装、调试过程说明
- 第六章 车模的各项参数

## 精读状态
- 扫描卡