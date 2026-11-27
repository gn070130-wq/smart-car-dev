# 雁过留痕_安徽师范大学_一往无前队
- group: 本科·雁过留痕
- chars: 35210
- source: `雁过留痕_安徽师范大学_一往无前队.txt`

## 摘要片段
摘要 本文系统介绍了一往无前队在二十一届全国大学生智能汽车竞赛雁过留痕 组中的系统方案。本次大赛采用大赛组委会统一指定的 C 车模，以 STC32G 为核 心控制器，一个总转分摄像头作为信息采集，再通过逐飞编码器检测智能车的实 时速度，使用，陀螺仪 imu660ra 获得车的偏角，使用舵机 SU400 控制车的偏转， 使用 PID 控制算法检测电机的转速使，从而实现智能车载运动过程中转向和速度 的闭环控制。为了提高车的稳定性，进行了大量的硬件与软件调试。 为保证巡线的稳定性，我们还进行了中线提取，丢线补偿，特殊元素判断等 算法。在打靶方面，我们使用了特殊的元素判断，单独判断靶纸的位置，然后通 过位置的不一样性判断多激光打靶。 关键词：智能车，雁过留痕，PID 闭环控制 4 目录 摘要 .........................................................................

## 技术信号
- **mcu**: STC32G, STC32G144K246, STC32
- **camera**: MT9V03X, mt9v03x
- **control**: 差速, 增量式, 位置式
- **vision**: 动态阈值, 最长白列, 差比和, 边线
- **laser**: 打靶, 激光, 云台
- **extra**: 编码器, imu660ra, imu660ra_get_gyro, imu660ra_gyro_x, imu660ra_gyro_y, imu660ra_gyro_z
- **element**: 十字, 环岛, 坡道, 斑马

## 代码/符号锚点（报告中出现的标识符）
CheckRow90GrayProtect, DiffSumCalc, ENCODER_DIR_1, ENCODER_DIR_2, ENCODER_DIR_DIR_1, ENCODER_DIR_DIR_2, ENCODER_DIR_PULSE_1, ENCODER_DIR_PULSE_2, FindBlackRing75, FindLongestWhiteColumnVertex, Init_Gyro_Offset, ShrinkCheck, SingleLineScan, atan, avoid_left, avoid_right, binarize_and_display, binarize_image, camera_init, car_image, compensate, display_binary_image, distance, encoder_clear_count, encoder_get_count, fabs, gpio_init, gpio_set_level, image_show, imu660ra_get_gyro, integral, ips200_draw_point, ips200_show_float, ips200_show_gray_image, ips200_show_int32, ips200_show_string, ips200_show_uint8, motor1, motor2, motor_init

## 结构线索
- 低（约 1.5 g/cm³）、抗弯模量高（可达 120 GPa）的优点，同等刚度下重量仅
- 最大行驶速度 约 3.2m/s
- float err1 = 0, l_err1 = 0 ,ki1 = 3.2 , kp1 = 29,speed_out1 = 0; //11.7v对应 3.15
- 29 12.6v对应 2.9 28
- float err2 = 0, l_err2 = 0 ,ki2 = 3.2 , kp2 = 29,speed_out2 = 0;
- float y_p = 0.9225;
- distance_out = distance_out + 0.01*(speed1 + speed2);
- x = dir_out*0.00174;
- y = (x/(1 - 0.3333*x*x))*y_p;
- 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, /* 0-9 */
- 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, /* 10-19 */
- 0.0, 0.0, 0.0, 0.0, 0.0, /* 20-24 */
- 100.0, /* 40 */
- 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, /* 41-50
- 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, /* 51-60
- 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, /* 61-70
- 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, 100.0, /* 71-80
- 100.0 /*
- float x_cur = 93.5f;
- float y_cur = 80.0f;
- float preaim_error = 0.0f;
- static float quad = 0.0f;
- x_cur = x_cur + 0.25*(compensate_add + NEAR_ROW - 30);
- x_cur = x_cur + 0.25*(compensate_add - NEAR_ROW + 30);
- preaim_error= atan((93.5f - x_cur) / (181 - y_cur)) * 57.2958f;

## 精读状态
- 扫描卡