#include "direction.h"  

// POS PID
struct PID_Pos gyro_dir_inner;
struct PID_Pos differ_dir_outer;
// 角速度环 set_speed = 80时gyro_Kp = 0.32，gyro_Ki = 0.0，gyro_Kd = 0.1
float gyro_Kp = 1.24; // 1.24
float gyro_Ki = 0.005; // 0.005
float gyro_Kd = 1.2; // 1.2
// 角度环 set_speed38 80时a1.8Ki = 0.0，angle_Kd = 16.0
float angle_Kp = 2.4; // 2.4
float angle_Ki = 0.0;
float angle_Kd = 1.0; // 2.0
float angle_island_Kp = 2.6; // 2.4
float angle_island_Ki = 0.0;
float angle_island_Kd = 2.0; // 2.0
// 偏差环 set_speed = 80时differ_Kp1 = 0.0，differ_Kp2 = 1.0，differ_Kd = 0.0
#if 0 == USE_SPEED_DECISION
float differ_Kp1[10] = {0.0, 0.1, 0.2, 0.2, 0.4, 0.0, 0.0, 0.0, 0.0, 0.0};
float differ_Kp2[10] = {1.0, 1.1, 1.2, 1.3, 1.5, 1.0, 1.0, 1.0, 1.0, 1.0};
float differ_Kp3[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // 近端
float differ_Kd[10] = {1.0, 1.0, 1.0, 1.0, 1.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#elif 1 == USE_SPEED_DECISION
float differ_Kp1[10] = {0.0, 0.1, 0.1, 0.1, 0.1, 0.0, 0.0, 0.0, 0.0, 0.0}; 
float differ_Kp2[10] = {1.0, 1.1, 1.2, 1.2, 1.2, 1.0, 1.0, 1.0, 1.0, 1.0};
float differ_Kp3[10] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0}; // 近端
float differ_Kd[10] = {1.0, 1.0, 1.5, 2.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
#endif 
// gyroscope
struct gyroscope gyro;
struct low_pass_filter gyro_yaw_filter;
float imu_gyro_x = 0.0f, imu_gyro_y = 0.0f, imu_gyro_z = 0.0f;
float imu_acc_x = 0.0f, imu_acc_y = 0.0f, imu_acc_z = 0.0f;
struct quaternion_solution gyro_quaternion =
{
    .acc_roll = 0.0f,
    .acc_pitch = 0.0f,
    .acc_yaw = 0.0f,
    .gyro_roll = 0.0f,
    .gyro_pitch = 0.0f,
    .gyro_yaw = 0.0f,
    .q0 = 1.0f,
    .q1 = 0.0f,
    .q2 = 0.0f,
    .q3 = 0.0f,
};
float I_ex, I_ey, I_ez;
float param_Kp = 0.17; // 0.17
float param_Ki = 0.004; // 0.004
struct gyro_deal_element gyro_element =
{
    .island = 0.0f,
    .model_turn_angle = 0.0f,
    .model_return_angle = 0.0f,
};

/****************************** direction pid ******************************/
/**
 * @brief 初始化转向环PID
 * @param void
 * @return void
 * @note 初始化转向环PID，设置Kp、Ki、Kd
 * @author YuYu
 */
void dir_inner_init(void)
{
    gyro_dir_inner.Kp = gyro_Kp;
    gyro_dir_inner.Ki = gyro_Ki;
    gyro_dir_inner.Kd = gyro_Kd;
    gyro_dir_inner.err = 0.0;
    gyro_dir_inner.last_err = 0.0;
    gyro_dir_inner.proportion = 0.0;
    gyro_dir_inner.integral = 0.0;
    gyro_dir_inner.differential = 0.0;
    gyro_dir_inner.out = 0.0;
    gyro_dir_inner.actual_out = 0.0;
    gyro_dir_inner.limit_integral = set_speed * 0.6; // 转向环积分限幅 set_speed * 0.6
    gyro_dir_inner.limit_actual_out = set_speed * 0.8; // 转向环限幅 set_speed * 0.8
}

/**
 * @brief 转向环PID
 * @param void
 * @return void
 * @note 转向环PID，根据陀螺仪数据计算转向角度
 * @author YuYu
 */
void dir_inner_pid(void)
{
    float actual_filter = lowpass_filter(&gyro_yaw_filter, (imu_dev.get_gyro_z() - gyro.init_velocity.gyro_yaw), 0.35); // [修正] 恢复滤波系数到0.35，平衡响应速度
    gyro_dir_inner.set = differ_dir_outer.actual_out; // 30.0（differ_dir_outer.actual_out）为基础图像偏差来确定 gyro_dir_inner.Kp, gyro_dir_inner.Kd 
    gyro_dir_inner.actual = actual_filter/28.6;
    gyro_dir_inner.err = gyro_dir_inner.set - gyro_dir_inner.actual;
    gyro_dir_inner.proportion = gyro_dir_inner.Kp * gyro_dir_inner.err;
    gyro_dir_inner.integral += gyro_dir_inner.Ki * gyro_dir_inner.err;
    gyro_dir_inner.integral = limit_value(gyro_dir_inner.integral, gyro_dir_inner.limit_integral, -gyro_dir_inner.limit_integral);
    gyro_dir_inner.differential = gyro_dir_inner.Kd * (gyro_dir_inner.err - gyro_dir_inner.last_err);
    gyro_dir_inner.last_err = gyro_dir_inner.err;
    gyro_dir_inner.out = gyro_dir_inner.proportion + gyro_dir_inner.integral + gyro_dir_inner.differential;
    gyro_dir_inner.actual_out = limit_value(gyro_dir_inner.out, gyro_dir_inner.limit_actual_out, -gyro_dir_inner.limit_actual_out);
}

/**
 * @brief 初始化偏差环PID
 * @param void
 * @return void
 * @note 初始化偏差环PID，设置Kp、Ki、Kd
 * @author YuYu
 */
void differ_outer_init(void)
{
    differ_dir_outer.Kp = angle_Kp;
    differ_dir_outer.Ki = angle_Ki;
    differ_dir_outer.Kd = angle_Kd;
    differ_dir_outer.err = 0.0;
    differ_dir_outer.last_err = 0.0;
    differ_dir_outer.proportion = 0.0;
    differ_dir_outer.integral = 0.0;
    differ_dir_outer.differential = 0.0;
    differ_dir_outer.out = 0.0;
    differ_dir_outer.actual_out = 0.0;
    differ_dir_outer.limit_actual_out = PID_DIFFER_MAX; // 偏差环限幅
}

/**
 * @brief 偏差环PID
 * @param void
 * @return void
 * @note 偏差环PID，根据图像偏差计算角度
 * @author YuYu
 */
void differ_outer_pid(void)
{
    if(judge_island.left_island_type != 0 || judge_island.right_island_type != 0)
    {
        differ_dir_outer.Kp = angle_island_Kp;
        differ_dir_outer.Ki = angle_island_Ki;
        differ_dir_outer.Kd = angle_island_Kd;
    }
    else
    {
        differ_dir_outer.Kp = angle_Kp;
        differ_dir_outer.Ki = angle_Ki;
        differ_dir_outer.Kd = angle_Kd;
    }
    differ_dir_outer.err = angle_line.differ_far;
    differ_dir_outer.proportion = differ_dir_outer.Kp * differ_dir_outer.err;
    differ_dir_outer.integral += differ_dir_outer.Ki * differ_dir_outer.err;
    differ_dir_outer.differential = differ_dir_outer.Kd * (differ_dir_outer.err - differ_dir_outer.last_err);
    differ_dir_outer.last_err = differ_dir_outer.err;
    differ_dir_outer.out = differ_dir_outer.proportion + differ_dir_outer.integral + differ_dir_outer.differential;
    differ_dir_outer.actual_out = limit_value(differ_dir_outer.out, differ_dir_outer.limit_actual_out, -differ_dir_outer.limit_actual_out);
}
/****************************** direction pid ******************************/

/****************************** gyroscope ******************************/
/**
 * @brief 初始化陀螺仪
 * @param void
 * @return void
 * @note 初始化陀螺仪，设置零点漂移
 * @author YuYu
 */
void gyroscope_init(void)
{
    int16_t gyro_count = 0;
    // 计算陀螺仪零点漂移
    for(int16_t i = 0; i < GYROSCOPE_INIT_COUNT; i++)
    {
        // 获取陀螺仪原始值数据
        imu_gyro_x = imu_dev.get_gyro_x();
        imu_gyro_y = imu_dev.get_gyro_y();
        imu_gyro_z = imu_dev.get_gyro_z();
        imu_acc_x = imu_dev.get_acc_x();
        imu_acc_y = imu_dev.get_acc_y();
        imu_acc_z = imu_dev.get_acc_z();
        if(abs(imu_gyro_x) < GYRO_STANDARD_DATA || abs(imu_gyro_y) < GYRO_STANDARD_DATA || abs(imu_gyro_z) < GYRO_STANDARD_DATA)
        {
            gyro_count++;
            gyro.init_velocity.acc_roll += imu_acc_x;
            gyro.init_velocity.acc_pitch += imu_acc_y;
            gyro.init_velocity.acc_yaw += imu_acc_z;
            gyro.init_velocity.gyro_roll += imu_gyro_x;
            gyro.init_velocity.gyro_pitch += imu_gyro_y;
            gyro.init_velocity.gyro_yaw += imu_gyro_z;
        }
        system_delay_ms(2);
    }
    // 计算陀螺仪零点漂移
    if(gyro_count < GYROSCOPE_INIT_COUNT/5)
    {
        gyro.init_velocity.acc_roll = 0.0;
        gyro.init_velocity.acc_pitch = 0.0;
        gyro.init_velocity.acc_yaw = 0.0;
        gyro.init_velocity.gyro_roll = 0.0;
        gyro.init_velocity.gyro_pitch = 0.0;
        gyro.init_velocity.gyro_yaw = 0.0;
        printf("gyroscope initial error.\r\n");
    }
    else
    {
        gyro.init_velocity.gyro_roll /= gyro_count;
        gyro.init_velocity.gyro_pitch /= gyro_count;
        gyro.init_velocity.gyro_yaw /= gyro_count;
        gyro.init_velocity.acc_roll /= gyro_count;
        gyro.init_velocity.acc_pitch /= gyro_count;
        gyro.init_velocity.acc_yaw /= gyro_count;
        printf("gyroscope initial ok.\r\n");
    }
}

/**
 * @brief 获取陀螺仪数据
 * @param void
 * @return void
 * @note 获取陀螺仪数据，包括加速度、角速度、磁力计
 * @author YuYu
 */
void gyroscope_get_data(void)
{
    // 获取陀螺仪数据
    gyro.angular_acceleration.roll = (imu_dev.get_acc_x() - gyro.init_velocity.acc_roll)/ACC_SCALE_FACTOR;
    if(fabs(gyro.angular_acceleration.roll) < ACC_STANDARD_DATA)
    {
        gyro.angular_acceleration.roll = 0.0f;
    }
    gyro.angular_acceleration.pitch = (imu_dev.get_acc_y() - gyro.init_velocity.acc_pitch)/ACC_SCALE_FACTOR;
    if(fabs(gyro.angular_acceleration.pitch) < ACC_STANDARD_DATA)
    {
        gyro.angular_acceleration.pitch = 0.0f;
    }
    gyro.angular_acceleration.yaw = (imu_dev.get_acc_z() - gyro.init_velocity.acc_yaw)/ACC_SCALE_FACTOR;
    if(fabs(gyro.angular_acceleration.yaw) < ACC_STANDARD_DATA)
    {
        gyro.angular_acceleration.yaw = 0.0f;
    }
    gyro.angular_velocity.roll = (imu_dev.get_gyro_x() - gyro.init_velocity.gyro_roll)/GYRO_SCALE_FACTOR;
    if(fabs(gyro.angular_velocity.roll) < GYRO_STANDARD_DATA)
    {
        gyro.angular_velocity.roll = 0.0f;
    }
    gyro.angular_velocity.pitch = (imu_dev.get_gyro_y() - gyro.init_velocity.gyro_pitch)/GYRO_SCALE_FACTOR;
    if(fabs(gyro.angular_velocity.pitch) < GYRO_STANDARD_DATA)
    {
        gyro.angular_velocity.pitch = 0.0f;
    }
    gyro.angular_velocity.yaw = (imu_dev.get_gyro_z() - gyro.init_velocity.gyro_yaw)/GYRO_SCALE_FACTOR;
    if(fabs(gyro.angular_velocity.yaw) < GYRO_STANDARD_DATA)
    {
        gyro.angular_velocity.yaw = 0.0f;
    }
    if(DEV_IMU963RA == imu_dev.imu_type) // 九轴
    {
        gyro.magnetometer.roll = imu_dev.get_mag_x();
        gyro.magnetometer.pitch = imu_dev.get_mag_y();
        gyro.magnetometer.yaw = imu_dev.get_mag_z();
    }
    // 四元数解算
    gyro_quaternion.acc_roll = ((float)gyro.angular_acceleration.roll * GYROSCOPE_FILTER_ALPHA) + gyro_quaternion.acc_roll * (1 - GYROSCOPE_FILTER_ALPHA);
    gyro_quaternion.acc_pitch = ((float)gyro.angular_acceleration.pitch * GYROSCOPE_FILTER_ALPHA) + gyro_quaternion.acc_pitch * (1 - GYROSCOPE_FILTER_ALPHA);
    gyro_quaternion.acc_yaw = ((float)gyro.angular_acceleration.yaw * GYROSCOPE_FILTER_ALPHA) + gyro_quaternion.acc_yaw * (1 - GYROSCOPE_FILTER_ALPHA);
    gyro_quaternion.gyro_roll = ((float)gyro.angular_velocity.roll * MY_PI/180);
    gyro_quaternion.gyro_pitch = ((float)gyro.angular_velocity.pitch * MY_PI/180);
    gyro_quaternion.gyro_yaw = ((float)gyro.angular_velocity.yaw * MY_PI/180);
    gyroscope_deal_element();
}

/**
 * @brief 四元数解算
 * @param float gx 陀螺仪角速度X轴
 * @param float gy 陀螺仪角速度Y轴
 * @param float gz 陀螺仪角速度Z轴
 * @param float ax 加速度X轴
 * @param float ay 加速度Y轴
 * @param float az 加速度Z轴
 * @return void
 * @note 四元数解算，根据陀螺仪数据和加速度数据更新四元数
 * @author YuYu
 */
void gyroscope_update(float gx, float gy, float gz, float ax, float ay, float az)
{
    // 计算四元数微分方程中的半周期系数
    float halfT = 0.5 * delta_T;
    // 姿态估算出的重力向量、姿态误差向量
    float vx, vy, vz;
    float ex, ey, ez;
    // 获取当前的姿态四元数
    float q0 = gyro_quaternion.q0;
    float q1 = gyro_quaternion.q1;
    float q2 = gyro_quaternion.q2;
    float q3 = gyro_quaternion.q3;
    // 预计算四元数乘积
    float q0q0 = q0 * q0;
    float q0q1 = q0 * q1;
    float q0q2 = q0 * q2;
    float q1q1 = q1 * q1;
    float q1q3 = q1 * q3;
    float q2q2 = q2 * q2;
    float q2q3 = q2 * q3;
    float q3q3 = q3 * q3;
    // 计算加速度向量的模长
    float norm = fast_sqrt(ax * ax + ay * ay + az * az);
    ax = ax * norm;
    ay = ay * norm;
    az = az * norm;
    // 从四元数构成的旋转矩阵中提取出重力方向
    vx = 2 * (q1q3 - q0q2);
    vy = 2 * (q0q1 + q2q3);
    vz = q0q0 - q1q1 - q2q2 + q3q3;
    // 衡量姿态误差
    ex = ay * vz - az * vy;
    ey = az * vx - ax * vz;
    ez = ax * vy - ay * vx;
    // 积分项累加误差，用于消除稳态误差
    I_ex += halfT * ex;
    I_ey += halfT * ey;
    I_ez += halfT * ez;
    float integral_limit = 0.05f;
    I_ex = limit_value(I_ex, integral_limit, -integral_limit);
    I_ey = limit_value(I_ey, integral_limit, -integral_limit);
    I_ez = limit_value(I_ez, integral_limit, -integral_limit);
    // 修正后的角速度 = 原始角速度 + 比例修正 + 积分修正
    gx = gx + param_Kp * ex + param_Ki * I_ex;
    gy = gy + param_Kp * ey + param_Ki * I_ey;
    gz = gz + param_Kp * ez + param_Ki * I_ez;
    // 使用修正后的角速度(gx, gy, gz)和一阶龙格-库塔法（本质上是欧拉法）来更新四元数
    q0 = q0 + (-q1 * gx - q2 * gy - q3 * gz) * halfT;
    q1 = q1 + (q0 * gx + q2 * gz - q3 * gy) * halfT;
    q2 = q2 + (q0 * gy - q1 * gz + q3 * gx) * halfT;
    q3 = q3 + (q0 * gz + q1 * gy - q2 * gx) * halfT;
    // 归一化四元数
    norm = fast_sqrt(q0 * q0 + q1 * q1 + q2 * q2 + q3 * q3);
    gyro_quaternion.q0 = q0 * norm;
    gyro_quaternion.q1 = q1 * norm;
    gyro_quaternion.q2 = q2 * norm;
    gyro_quaternion.q3 = q3 * norm;
}

/**
 * @brief 获取陀螺仪角度
 * @param void
 * @return void
 * @note 获取陀螺仪角度，包括滚转、俯仰、偏航
 * @author YuYu
 */
void gyro_get_angle(void)
{
    // gyroscope_get_data();
    gyroscope_update(gyro_quaternion.gyro_roll, gyro_quaternion.gyro_pitch, gyro_quaternion.gyro_yaw, gyro_quaternion.acc_roll, gyro_quaternion.acc_pitch, gyro_quaternion.acc_yaw);
    float q0 = gyro_quaternion.q0;
    float q1 = gyro_quaternion.q1;
    float q2 = gyro_quaternion.q2;
    float q3 = gyro_quaternion.q3;
    gyro.angle.roll = atan2(2 * q2 * q3 + 2 * q0 * q1, -2 * q1 * q1 - 2 * q2 * q2 + 1) * 180 / MY_PI;
    gyro.angle.pitch = asin(-2 * q1 * q3 + 2 * q0 * q2) * 180 / MY_PI;
    gyro.angle.yaw = atan2(2 * q1 * q2 + 2 * q0 * q3, -2 * q2 * q2 - 2 * q3 * q3 + 1) * 180 / MY_PI;
}

/**
 * @brief 处理陀螺仪数据
 * @param void
 * @return void
 * @note 处理陀螺仪数据，包括元素积分
 * @author YuYu
 */
void gyroscope_deal_element(void) 
{
    // 环岛积分
    if(judge_island.left_island_type >= 2 || judge_island.right_island_type >= 2)
    {
        gyro_get_angle();
        static float last_yaw = 0.0f;
        float delta_angle = gyro.angle.yaw - last_yaw;
        if(delta_angle > 180.0f)
        {
            delta_angle -= 360.0f;
        }
        else if(delta_angle < -180.0f)
        {
            delta_angle += 360.0f;
        }
        last_yaw = gyro.angle.yaw;
        gyro_element.island += delta_angle;
    }
    // 转向积分
    if(model_control.turn_flag == true)
    {
        gyro_element.model_turn_angle += gyro.angular_velocity.yaw * 0.01f;
    }
    else if(model_control.return_flag == true)
    {
        gyro_element.model_return_angle += gyro.angular_velocity.yaw * 0.01f;
    }
}

/**
 * @brief 快速平方根
 * @param float x
 * @return float
 * @note 快速平方根算法
 * @author YuYu
 */
float fast_sqrt(float x)
{
    float halfx = 0.5f * x;
    float y = x;
    long i;
    memcpy(&i, &y, sizeof(float));
    i = 0x5f3759df - (i >> 1);
    memcpy(&y, &i, sizeof(float));
    y = y * (1.5f - (halfx * y * y));
    return y;
}
/****************************** gyroscope ******************************/
