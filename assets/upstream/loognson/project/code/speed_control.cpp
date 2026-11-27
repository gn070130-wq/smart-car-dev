#include "speed_control.h"

// 编码器初始化
zf_driver_encoder encoder_dir_1(ENCODER_DIR_1_PATH);
zf_driver_encoder encoder_dir_2(ENCODER_DIR_2_PATH);
struct encoder_struct encoder =
{
    .left = 0,
    .right = 0
};
// 电机初始化
zf_driver_gpio  drv8701e_dir_1(DIR_1_PATH, O_RDWR);
zf_driver_gpio  drv8701e_dir_2(DIR_2_PATH, O_RDWR);
zf_driver_pwm   drv8701e_pwm_1(PWM_1_PATH);
zf_driver_pwm   drv8701e_pwm_2(PWM_2_PATH);
int16_t set_speed = 140; // 80(0) 100(1) 120(2) 140(3) 160(4) 170(5) 180(6) 190(7) 200(8)
int16_t constant_speed = 0;
struct departure_struct departure =
{
    .start_pwm = 0,             // 初始PWM
    .limit_start_pwm = 1500,    // 初始初始PWM
    .pwm_max = 0,               // 最大PWM
    .limit_pwm_max = 0          // 最大PWM最大值
};
struct motor_stop stop =
{
    .stop_flag = 0,             // 停止标志位
    .finish_stop_flag = 0,      // 斑马线停止标志位
    .detect_RGB_red = false,    // 检测红色标志位
    .misjudge_flag = false      // 错误判断标志位
};
uint8_t integral_limit_flag = 0;// 积分限制标志位
struct model_control_speed model_control = 
{
    .block_x = 0,                   // 模型控制x坐标
    .block_y = 0,                   // 模型控制y坐标
    .middle_end_row = WANT_END_ROW, // 中间结束行坐标
    .middle_end_column = 0,         // 中间结束列坐标
    .turn_flag = false,             // 转弯标志位
    .return_flag = false            // 返回标志位
};
// 堵转保护
int16_t duzhuan_count = 0; // 堵转保护计数
// PID初始化
struct PID_Inc right_speed = {0}, left_speed = {0};
// 滤波初始化
struct moving_filter filter_left = {0};
struct moving_filter filter_right = {0};
// BLDC控制初始化
struct BLDC_control_struct BLDC_control = 
{
    .start_lock = false,        // BLDC发车锁
    .bldc_constant_pwm = 800    // BLDC恒定PWM
};
// 速度决策初始化
struct speed_decision_struct decision = {0};

/****************************** motor ******************************/
/**
 * @brief 电机初始化
 * @param void
 * @return void
 * @note 电机初始化
 * @author YuYu
 */
void motor_init(void)
{  
    // 电机初始化
    drv8701e_pwm_1.get_dev_info(&drv8701e_pwm_1_info);
    drv8701e_pwm_2.get_dev_info(&drv8701e_pwm_2_info);
    constant_speed = set_speed;
    // 滤波初始化
    moving_filter_init(&filter_left, 0);
    moving_filter_init(&filter_right, 0);
    #if 1 == USE_SPEED_DECISION // 速度决策初始化
    decision.last_speed = constant_speed;
    decision.max_speed = (int16_t)(constant_speed * MAX_SPEED_COEFFICIENT);
    decision.min_speed = (int16_t)(constant_speed * MIN_SPEED_COEFFICIENT);
    #endif
    printf("motor initial ok\r\n");
}

/**
 * @brief 速度控制
 * @param void
 * @return void
 * @note 速度控制
 * @author YuYu
 */
void speed_control(void)
{
    int16_t left = encoder_dir_1.get_count();
    int16_t right = -encoder_dir_2.get_count();
    encoder_dir_1.clear_count();
    encoder_dir_2.clear_count();
    encoder.left = moving_filter_calculate(&filter_left, left);
    encoder.right = moving_filter_calculate(&filter_right, right);
    if(integral_limit_flag == 0)
    {
        motor_departure(2000);
    }
    else
    {
        speed_calculate_dir();
        left_speed.actual = (float)encoder.left;
        right_speed.actual = (float)encoder.right;
        PID_increment(&left_speed);
        PID_increment(&right_speed);
        motor_control(5000);
    }
}

/**
 * @brief 发车
 * @param max_acc_pwm 最大加速度PWM
 * @return void
 * @note 发车，根据最大加速度PWM和当前速度计算PWM变化
 * @author YuYu
 */
void motor_departure(int16_t max_acc_pwm)
{
    static int16_t time_count = 0; 
    // 设置初始pwm和最大pwm
    departure.start_pwm = set_speed * 10;
    departure.start_pwm = limit_value(departure.start_pwm, departure.limit_start_pwm, -(departure.limit_start_pwm));
    departure.limit_pwm_max = departure.start_pwm + max_acc_pwm;
    // 设置加速时间
    time_count++;
    if(time_count <= SYSTEM_ACC_TIME_MAX)
    {
        int16_t acc_step = (int16_t)((int32_t)(time_count * max_acc_pwm) / SYSTEM_ACC_TIME_MAX);
        left_speed.out = (float)(departure.start_pwm + acc_step);
        right_speed.out = (float)(departure.start_pwm + acc_step);
        motor_control(departure.limit_pwm_max);
    }
    else
    {
        time_count = 0;
        integral_limit_flag = 1;        // 退出发车限幅
        stop.stop_flag = 1;             // 退出发车误判
        judge_zebra_line.flag = 1;      // 退出斑马线发车误判
        left_speed.out = 0.0;
        right_speed.out = 0.0;
        left_speed.integral = 0.0;  // 清零积分项
        right_speed.integral = 0.0;
    }
}

/**
 * @brief 设定速度控制
 * @param void
 * @return void
 * @note 设定速度控制
 * @author YuYu
 */
void speed_calculate_dir(void)
{
    if(stop.stop_flag == 2 && stop.finish_stop_flag == 0) // 紧急状况下停车
    {
        static int16_t stop_count = 0;
        left_speed.set = 0.0;
        right_speed.set = 0.0;
        stop_count++;
        if(stop_count >= 50) // T = T0 * 50
        {
            stop.finish_stop_flag = 1;
            stop_count = 0;
        }
    }
    else if(judge_zebra_line.flag == 2) // 斑马线停车
    {
        static int16_t zebra_line_count = 0;
        zebra_line_count++;
        if(zebra_line_count >= 100) // T = T0 * 100
        {
            stop.stop_flag = 2;
            zebra_line_count = 0;
        }
    }
    else
    {
        #if 1 == USE_SPEED_DECISION
            set_speed = speed_decision(); // 速度决策
        #endif
        // 色块识别后的转向控制
        if(stop.detect_RGB_red == true && model_control.turn_flag == false && model_control.return_flag == false)
        {
            float left_pulse = set_speed * DETECT_DECREASE_SPEED; // 红色色块减速速度系数
            float right_pulse = set_speed * DETECT_DECREASE_SPEED; // 红色色块减速速度系数
            if(gyro_dir_inner.actual_out > 0) // 偏右
            {
                left_speed.set = left_pulse - gyro_dir_inner.actual_out;
                right_speed.set = right_pulse;
            }
            else if(gyro_dir_inner.actual_out < 0) // 偏左
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse + gyro_dir_inner.actual_out;
            }
            else
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse;
            }
            if(model_type != 0 && model_type != last_model_type)
            {
                last_model_type = model_type;
                model_type = 0;
                model_coord_flag = 0;
                stop.misjudge_flag = true;
                stop.detect_RGB_red = false;
                model_control.turn_flag = true;
                // 清零 PID 积分项，保证每次转向起点一致
                gyro_dir_inner.integral = 0.0f;
                gyro_dir_inner.last_err = 0.0f;
                differ_dir_outer.integral = 0.0f;
                differ_dir_outer.last_err = 0.0f;
                // printf("\033[1;33m[DEBUG]\033[0m model type: %d\n", last_model_type);
            }
        }
        else if(model_control.turn_flag == true)
        {
            float left_pulse = set_speed * TURN_SPEED;
            float right_pulse = set_speed * TURN_SPEED;
            uint8_t model_location_differ = abs(model_control.block_x - START_MIDDLE_COLUMN);
            if(last_model_type == 1) // 左转
            {
                float turn_angle = 0.0f;
                if(model_control.middle_end_column < -20) // 左弯
                {
                    turn_angle = 38.0f + model_location_differ * 1.0f;
                    turn_angle = limit_value(turn_angle, 82.0f, 38.0f);
                }
                else if(model_control.middle_end_column > 20) // 右弯
                {
                    turn_angle = 20.0f + model_location_differ * 0.5f;
                    turn_angle = limit_value(turn_angle, 60.0f, 20.0f);
                }
                else
                {
                    turn_angle = 34.0f + model_location_differ * 1.0f;
                    turn_angle = limit_value(turn_angle, 72.0f, 34.0f);
                }
                // printf("\033[1;33m[DEBUG]\033[0m turn_angle: %f\n", turn_angle);
                // 退出转向
                if(fabs(gyro_element.model_turn_angle) > turn_angle)
                {
                    model_control.turn_flag = false;
                    model_control.return_flag = true;
                    gyro_element.model_turn_angle = 0.0f;
                }
            }
            else if(last_model_type == 2) // 右转
            {
                float turn_angle = 0.0f;
                if(model_control.middle_end_column < -20) // 左弯
                {
                    turn_angle = 20.0f + model_location_differ * 0.5f;
                    turn_angle = limit_value(turn_angle, 60.0f, 20.0f);
                }
                else if(model_control.middle_end_column > 20) // 右弯
                {
                    turn_angle = 38.0f + model_location_differ * 1.0f;
                    turn_angle = limit_value(turn_angle, 82.0f, 38.0f);
                }
                else
                {
                    turn_angle = 34.0f + model_location_differ * 1.0f;
                    turn_angle = limit_value(turn_angle, 72.0f, 34.0f);
                }
                // printf("\033[1;33m[DEBUG]\033[0m turn_angle: %f\n", turn_angle);
                // 退出转向
                if(fabs(gyro_element.model_turn_angle) > turn_angle)
                {
                    model_control.turn_flag = false;
                    model_control.return_flag = true;
                    gyro_element.model_turn_angle = 0.0f;
                }
            }
            else if(last_model_type == 3) // 直行直接退出
            {
                model_control.turn_flag = false;
                model_control.return_flag = true;
                gyro_element.model_turn_angle = 0.0f;
            }
            float turn_gyro_differ = gyro_dir_inner.actual_out * TURN_DIFF_BOOST;
            turn_gyro_differ = limit_value(turn_gyro_differ, 0.7 * left_pulse, -0.7 * left_pulse);
            if(gyro_dir_inner.actual_out > 0) // 偏右
            {
                left_speed.set = left_pulse - turn_gyro_differ;
                right_speed.set = right_pulse;
            }
            else if(gyro_dir_inner.actual_out < 0) // 偏左
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse + turn_gyro_differ;
            }
            else
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse;
            }
        }
        else if(model_control.return_flag == true)
        {
            float left_pulse = set_speed * TURN_SPEED;
            float right_pulse = set_speed * TURN_SPEED;
            uint8_t model_location_differ = abs(model_control.block_x - START_MIDDLE_COLUMN);
            if(last_model_type == 1) // 左转
            {
                float return_angle = 0.0f;
                if(model_control.middle_end_column < -20) // 左弯 左转回正
                {
                    return_angle = 18.0f - model_location_differ * 1.0f;
                    return_angle = limit_value(return_angle, 22.0f, 15.0f);
                }
                else if(model_control.middle_end_column > 20) // 右弯 左转回正
                {
                    return_angle = 62.0f + model_location_differ * 2.0f;
                    return_angle = limit_value(return_angle, 128.0f, 62.0f);
                }
                else
                {
                    return_angle = 40.0f + model_location_differ * 1.0f;
                    return_angle = limit_value(return_angle, 80.0f, 40.0f);
                }
                // printf("\033[1;33m[DEBUG]\033[0m return_angle: %f\n", return_angle);
                // 退出转向
                if(fabs(gyro_element.model_return_angle) > return_angle)
                {
                    model_control.return_flag = false;
                    gyro_element.model_return_angle = 0.0f;
                    last_model_type = 0;
                }
            }
            else if(last_model_type == 2) // 右转
            {
                float return_angle = 0.0f;
                if(model_control.middle_end_column < -20) // 左弯 右转回正
                {
                    return_angle = 62.0f + model_location_differ * 2.0f;
                    return_angle = limit_value(return_angle, 128.0f, 62.0f);
                }
                else if(model_control.middle_end_column > 20) // 右弯 右转回正
                {
                    return_angle = 18.0f - model_location_differ * 1.0f;
                    return_angle = limit_value(return_angle, 22.0f, 15.0f);
                }
                else
                {
                    return_angle = 40.0f + model_location_differ * 1.0f;
                    return_angle = limit_value(return_angle, 80.0f, 40.0f);
                }
                // printf("\033[1;33m[DEBUG]\033[0m return_angle: %f\n", return_angle);
                // 退出转向
                if(fabs(gyro_element.model_return_angle) > return_angle)
                {
                    model_control.return_flag = false;
                    gyro_element.model_return_angle = 0.0f;
                    last_model_type = 0;
                }
            }
            else if(last_model_type == 3) // 直行直接退出
            {
                model_control.return_flag = false;
                gyro_element.model_return_angle = 0.0f;
                last_model_type = 0;
            }
            float turn_gyro_differ = gyro_dir_inner.actual_out * TURN_DIFF_BOOST; // 转向差补偿
            turn_gyro_differ = limit_value(turn_gyro_differ, 0.7 * left_pulse, -0.7 * left_pulse);
            if(gyro_dir_inner.actual_out > 0) // 偏右
            {
                left_speed.set = left_pulse - turn_gyro_differ;
                right_speed.set = right_pulse;
            }
            else if(gyro_dir_inner.actual_out < 0) // 偏左
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse + turn_gyro_differ;
            }
            else
            {
                left_speed.set = left_pulse;
                right_speed.set = right_pulse;
            }
        }
        else
        {
            if(gyro_dir_inner.actual_out > 0) // 偏右
            {
                left_speed.set = set_speed - gyro_dir_inner.actual_out;
                right_speed.set = set_speed;
            }
            else if(gyro_dir_inner.actual_out < 0) // 偏左
            {
                left_speed.set = set_speed;
                right_speed.set = set_speed + gyro_dir_inner.actual_out;
            }
            else
            {
                left_speed.set = set_speed;
                right_speed.set = set_speed;
            }
        }
    }
}

/**
 * @brief 电机控制
 * @param out_max 最大输出PWM
 * @return void
 * @note 电机控制
 * @author YuYu
 */
void motor_control(float out_max)
{
    // if(integral_limit_flag == 1)
    // {
    //     prevent_motor_rotation();
    // }
    if(stop.finish_stop_flag == 0)
    {
        left_speed.actual_out = left_speed.out;
        right_speed.actual_out = right_speed.out;
        left_speed.actual_out = limit_value(left_speed.actual_out, out_max, -out_max);
        right_speed.actual_out = limit_value(right_speed.actual_out, out_max , -out_max);
        uint16_t duty_left = (uint16_t)fabs(left_speed.actual_out);
        uint16_t duty_right = (uint16_t)fabs(right_speed.actual_out);
        if(left_speed.actual_out > 0) // 正转
        {
            drv8701e_dir_1.set_level(1);
            drv8701e_pwm_1.set_duty(duty_left);
        }
        else
        {
            drv8701e_dir_1.set_level(0);
            drv8701e_pwm_1.set_duty(duty_left);
        }
        if(right_speed.actual_out > 0) // 正转
        {
            drv8701e_dir_2.set_level(1);
            drv8701e_pwm_2.set_duty(duty_right);
        }
        else
        {
            drv8701e_dir_2.set_level(0);
            drv8701e_pwm_2.set_duty(duty_right);
        }
    }
    else
    {
        drv8701e_pwm_1.set_duty(0);
        drv8701e_pwm_2.set_duty(0);
    }
}

/**
 * @brief 防止电机过速
 * @param void
 * @return void
 * @note 防止电机过速
 * @author YuYu
 */
void prevent_motor_rotation(void)
{
    if((abs(encoder.left) < OVERLOAD_SPEED_THRESHOLD && abs(left_speed.actual_out) > DETECT_DECREASE_SPEED) 
        || (abs(encoder.right) < OVERLOAD_SPEED_THRESHOLD && abs(right_speed.actual_out) > DETECT_DECREASE_SPEED))
    {
        duzhuan_count++;
    }
    else
    {
        duzhuan_count--;
        if(duzhuan_count <= 0)
        {
            duzhuan_count = 0;
        }
    }
    if(duzhuan_count >= 100)
    {
        stop.stop_flag = 2;
    }
}

/**
 * @brief 电机控制停止
 * @param void
 * @return void
 * @note 电机控制停止
 * @author YuYu
 */
void motor_control_stop(void)
{
    if(stop.stop_flag == 1 && stop.detect_RGB_red == false)
    {
        const uint8_t check_row = (WANT_START_ROW - 5); // 检测行
        const uint8_t left_start = (START_MIDDLE_COLUMN - 5), left_end = (START_MIDDLE_COLUMN - 20);
        const uint8_t right_start = (START_MIDDLE_COLUMN + 5), right_end = (START_MIDDLE_COLUMN + 20);
        const uint8_t mid_col = START_MIDDLE_COLUMN;
        uint8_t left_found = 0; // 是否找到左边界
        uint8_t right_found = 0; // 是否找到右边界
        for(uint8_t j = left_start; j >= left_end; j--) // 扫描左侧区域是否有白点
        {
            if(image_buffer[check_row][j] == WHITE)
            {
                left_found = 1;
                break;
            }
        }
        for(uint8_t j = right_start; j <= right_end; j++) // 扫描右侧区域是否有白点
        {
            if(image_buffer[check_row][j] == WHITE)
            {
                right_found = 1;
                break;
            }
        }
        uint8_t mid_black = (image_buffer[check_row][mid_col] == BLACK);
        if(!left_found && !right_found && mid_black) // 如果左右都没找到边界, 且中间是黑的，认为跑出赛道
        {
            stop.stop_flag = 2;
        }
    }
}
/****************************** motor ******************************/

/****************************** PID ******************************/
/**
 * @brief PID初始化
 * @param void
 * @return void
 * @note PID初始化
 * @author YuYu
 */
void pid_inc_init(void)
{
    left_speed.Kp = 26.0;                           // p参数 18.0 20.0 26.0
    left_speed.Ki = 4.8;                            // i参数 3.0  7.5  4.8
    left_speed.Kd = 0.0;                            // d参数 6.0  19.0 0.0
    left_speed.pin = 0.0;                           // 前馈 2.0
    left_speed.err = 0.0;                           // 偏差值
    left_speed.last_err = 0.0;                      // 定义上一个偏差值
    left_speed.llast_err = 0.0;                     // 定义上上个偏差值
    left_speed.set  = 0.0;                          // 设定的速度
    left_speed.actual = 0.0;                        // 定义实际速度
    left_speed.increment  = 0.0;                    // 定义PID增量
    left_speed.proportion = 0.0;                    // 比例
    left_speed.integral = 0.0;                      // 积分  
    left_speed.differential = 0.0;                  // PID微分项
    left_speed.out = 0.0;                           // PID返回值，电机输出
    left_speed.limit_actual_out = 6000.0;

    right_speed.Kp = 23.0;                          // p参数 15.0 17.0 23.0
    right_speed.Ki = 4.1;                           // i参数 3.5  7.5  4.1
    right_speed.Kd = 0.0;                           // d参数 10.0 20.0 0.0
    right_speed.pin = 0.0;                          // 前馈 2.0
    right_speed.err = 0.0;                          // 偏差值
    right_speed.last_err = 0.0;                     // 定义上一个偏差值
    right_speed.llast_err = 0.0;                    // 定义上上个偏差值
    right_speed.set  = 0.0;                         //设定的速度
    right_speed.actual = 0.0;                       //定义实际速度
    right_speed.increment  = 0.0;                   // 定义PID增量
    right_speed.proportion = 0.0;                   // 比例
    right_speed.integral = 0.0;                     // 积分
    right_speed.differential = 0.0;                 // PID微分项
    right_speed.out = 0.0;                          // PID返回值，电机输出
    right_speed.limit_actual_out = 6000.0;
}

/**
 * @brief PID增量
 * @param PID PID结构体指针
 * @return void
 * @note PID增量
 * @author YuYu
 */
void PID_increment(PID_Inc *PID)
{
    // 更新误差
    PID->err = PID->set - PID->actual;                                                          // 偏差 = 期望 - 实际
    // PID计算
    PID->proportion = PID->Kp * (PID->err - PID->last_err);                                     // 比例 = Kp * (误差 - 上次误差)
    // if(fabs(PID->err) <= ERROR_INTEGRAL)                                                        // 积分分离
    // {
    //     PID->integral = PID->Ki * PID->err;                                                     // 积分 = Ki * 误差
    // }
    // else if(fabs(PID->err) <= 2 * ERROR_INTEGRAL)
    // {
    //     float radio = ((2.0f * ERROR_INTEGRAL) - fabs(PID->err))/ERROR_INTEGRAL;
    //     radio = limit_value(radio, 1.0f, 0.0f);
    //     PID->integral = PID->Ki * PID->err * radio;                                             // 积分 = Ki * 误差 * 积分系数
    // }
    // else
    // {
    //     PID->integral = 0.0;                                                                    // 大误差时关闭积分
    // }
    PID->integral = PID->Ki * PID->err;                                                         // 积分 = Ki * 误差
    PID->differential = PID->Kd * (PID->err - 2 * PID->last_err + PID -> llast_err);            // 微分 = Kd * (误差 - 2 * 上次误差 + 上上次误差)
    // 前馈控制
    float forward_increment = (PID->set - PID->last_set) * PID->pin;                            // 前馈增量
    forward_increment = limit_value(forward_increment, 600, -600);                              // 前馈限幅
    // 总体增量
    PID->increment = PID->proportion + PID->differential + PID->integral + forward_increment;   // 增量 = 比例 + 微分 + 积分 + 前馈增量
    PID->out += PID->increment;                                                                 // 输出 = 增量
    PID->out = limit_value(PID->out, PID->limit_actual_out, -(PID->limit_actual_out));          // 输出限幅
    // 更新数据
    PID->llast_err = PID->last_err;                                                             // 上上次误差 = 上次误差
    PID->last_err = PID->err;                                                                   // 上次误差 = 误差
    PID->last_set = PID->set;
}
/****************************** PID ******************************/

/****************************** filter ******************************/
/**
 * @brief 滤波初始化
 * @param filter 滤波结构体指针
 * @param initial_value 初始值
 * @return void
 * @note 滤波初始化
 * @author YuYu
 */
void moving_filter_init(moving_filter* filter, int16_t initial_value)
{
    if(filter == NULL)
    {
        return;
    }
    filter->index = 0;
	filter->sum = (int32_t)initial_value * FILTER_SIZE;
    for(uint8_t i = 0; i < FILTER_SIZE; i++)
    {
        filter->buffer[i] = initial_value;
    }
}

/**
 * @brief 滤波计算
 * @param filter 滤波结构体指针
 * @param refresh_value 刷新值
 * @return int16_t 滤波值
 * @note 滤波计算
 * @author YuYu
 */
int16_t moving_filter_calculate(moving_filter* filter, int16_t refresh_value)
{
    if(filter == NULL)
    {
        return refresh_value;
    }
    // 减去旧值，加上新值
    filter->sum = filter->sum - filter->buffer[filter->index] + refresh_value;
    // 更新缓冲区
    filter->buffer[filter->index] = refresh_value;
    // 更新索引 (环形缓冲)
    filter->index = (filter->index + 1) % FILTER_SIZE; 
    // 返回平均值
    return (int16_t)(filter->sum / FILTER_SIZE);
}

/**
 * @brief 低通滤波
 * @param flt 低通滤波结构体指针
 * @param input 输入值
 * @param alpha 低通滤波系数
 * @return float 低通滤波值
 * @note 低通滤波
 * @author YuYu
 */
float lowpass_filter(low_pass_filter* flt, float input, float alpha)
{
    float y = alpha * input + (1.0f - alpha) * flt->last_prev;
    flt->last_prev = y;
    return y;
}
/****************************** filter ******************************/

/****************************** BLDC control ******************************/
void bldc_control_start(void)
{
    uint16_t duty = BLDC_MIN_PWM; // BLDC起动PWM
    uint16_t count_max = (BLDC_control.bldc_constant_pwm - BLDC_MIN_PWM); // 从起动PWM增加到恒定PWM所需的步数
    for(uint16_t i = 0; i <= count_max; i++)
    {
        duty++;
        esc_set_duty(duty);
        system_delay_ms(10); // 每步增加的时间间隔，调整加速曲线
        if(duty == BLDC_control.bldc_constant_pwm)
        {
            break;
        }
    }
}
/****************************** BLDC control ******************************/

/****************************** speed decision ******************************/
/**
 * @brief 速度决策
 * @param void
 * @return int16_t 速度决策值
 * @note 速度决策
 * @author YuYu
 */
int16_t speed_decision(void)
{
    static float speed_differ = 0.0f;
    float new_differ = fabs(angle_line.differ_decision);
    // 非对称低通滤波：入弯快响应，出弯慢响应
    if(new_differ > speed_differ) // 偏差变大 → 入弯
    {
        speed_differ = 0.25f * new_differ + 0.75f * speed_differ;
    }
    else // 偏差变小 → 出弯/直道
    {
        speed_differ = 0.15f * new_differ + 0.85f * speed_differ;
    }
    // 偏差处理
    float differ = speed_differ;
    float radio = (differ - SPEED_DEAD_ZONE) / (float)(FAR_DIFFER_MAX - SPEED_DEAD_ZONE);
    radio = limit_value(radio, 1.0f, 0.0f);
    float curve = 1.0f - exp(-radio * 3.0f); // 速度曲线
    // 速度决策
    float desire_speed = 0.0f;
    if(judge_ramp.flag == true) // 坡道
    {
        desire_speed = (float)(constant_speed * RAMP_DECREASE_SPEED);
    }
    else if(judge_roadblock.left_roadblock_flag || judge_roadblock.right_roadblock_flag) // 避障
    {
        desire_speed = (float)constant_speed;
    }
    else if(judge_island.left_island_type || judge_island.right_island_type) // 环岛
    {
        // desire_speed = (float)decision.max_speed;
        if((judge_island.left_island_type == 1 || judge_island.left_island_type == 2) 
            || (judge_island.right_island_type == 1 || judge_island.right_island_type == 2))
        {
            desire_speed = (float)constant_speed;
        }
        else
        {
            desire_speed = (float)(decision.max_speed - (decision.max_speed - constant_speed) * curve);
        }
    }
    else if(fabs(angle_line.differ_far - angle_line.last_differ_far) < 4.0f && speed_differ < SPEED_DEAD_ZONE
        && verge.left_last_point.row < (WANT_END_ROW + 8) && verge.right_last_point.row < (WANT_END_ROW + 8)
        && verge.left_last_point.column > (LEFT_BOUNDARY_COLUMN + 28) && verge.right_last_point.column < (RIGHT_BOUNDARY_COLUMN - 28)) // 直道
    {
        float straight_curve = 1.0f - limit_value(speed_differ / SPEED_DEAD_ZONE, 1.0f, 0.0f);
        float straight_decrese = constant_speed * (STRAIGHT_SPEED - 1) * straight_curve;
        desire_speed = (float)(constant_speed * STRAIGHT_SPEED - straight_decrese);
    }
    else
    {
        desire_speed = (float)(decision.max_speed - (decision.max_speed - decision.min_speed) * curve);
    }
    // 平滑速度变化
    int16_t differ_speed = (int16_t)(desire_speed - decision.last_speed);
    if(differ_speed < 0) // 需要减速 → 允许更快降速
    {
        differ_speed = limit_value(differ_speed, 0, -DECREASE_MAX_SPEED);
    }
    else // 需要加速 → 保持原有速率
    {
        differ_speed = limit_value(differ_speed, INCREASE_MAX_SPEED, 0);
    }
    decision.target_speed = decision.last_speed + differ_speed;
    decision.last_speed = decision.target_speed;
    return decision.target_speed;
    // float radio = fabs(angle_line.differ_decision/(FAR_DIFFER_MAX));
    // if(radio > 1.0f || judge_ramp.flag == true)
    // {
    //     radio = 1.0f; // 限制最大比例，防止速度过低或负值
    // }
    // decision.target_speed = (int16_t)(decision.max_speed - (decision.max_speed - decision.min_speed) * radio);
    // return decision.target_speed;
}
/****************************** speed decision ******************************/
