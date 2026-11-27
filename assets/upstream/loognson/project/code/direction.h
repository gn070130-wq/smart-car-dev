#ifndef _DIRECTION_H_
#define _DIRECTION_H_

#include "zf_common_headfile.hpp"

/****************************** direction pid ******************************/
#define  ACC_SCALE_FACTOR           (4096.0f) // 加速度计缩放因子
#define  GYRO_SCALE_FACTOR          (14.35f) // 陀螺仪缩放因子
/****************************** gyroscope ******************************/
#define  GYROSCOPE_INIT_COUNT       (500) // 初始化速度采样次数
#define  ACC_STANDARD_DATA          (12.0) // 角加速度计标准数据
#define  GYRO_STANDARD_DATA         (10.0) // 角速度计标准数据
#define  GYROSCOPE_FILTER_ALPHA     (0.7f) // 低通滤波系数
#define  MY_PI                      (3.1415926f) // π
#define  delta_T                    (0.01f) // 时间间隔
/****************************** POS PID ******************************/
#define  SET_DIFFER_TYPE            (3) // set_speed以80为基础，每次增加20

/****************************** gyroscope ******************************/
struct dir // 陀螺仪姿态角结构体
{
    float roll;
    float pitch;
    float yaw;
};
struct data // 陀螺仪数据结构体
{
    float acc_roll;
    float acc_pitch;
    float acc_yaw;
    float gyro_roll;
    float gyro_pitch;
    float gyro_yaw;
};
struct gyroscope // 陀螺仪数据结构体
{
    data init_velocity;         // 零漂
    dir angular_acceleration;   // 角加速度
    dir angular_velocity;       // 角速度
    dir magnetometer;           // 磁力计
    dir angle;                  // 角度
};
extern struct gyroscope gyro;
extern float imu_acc_x, imu_acc_y, imu_acc_z;
extern float imu_gyro_x, imu_gyro_y, imu_gyro_z;
struct quaternion_solution // 四元数参数结构体
{
    float acc_roll;
    float acc_pitch;
    float acc_yaw;
    float gyro_roll;
    float gyro_pitch;
    float gyro_yaw;
    float q0;
    float q1;
    float q2;
    float q3;
};
extern struct quaternion_solution gyro_quaternion;
struct gyro_deal_element // 陀螺仪处理元素结构体
{
    float island;
    float model_turn_angle;
    float model_return_angle;
};
extern struct gyro_deal_element gyro_element;
/****************************** POS PID ******************************/
struct PID_Pos // 位置式PID
{
    float Kp;                       // Kp
    float Ki;                       // Ki
    float Kd;                       // Kd
    float pin;                      // 前馈
    float set;                      // 设定值
    float actual;                   // 实际值
    float err;                      // 偏差值
    float last_err;                 // 上次偏差值
    float proportion;               // 比例
    float integral;                 // 积分
    float differential;             // 微分
    float limit_integral;           // 积分限幅
    float limit_differential;       // 微分限幅
    float limit_actual_out;         // 输出限幅
    float out;                      // PID输出
    float actual_out;               // 实际输出
};
extern struct PID_Pos gyro_dir_inner, differ_dir_outer;
extern float angle_Kp; // 角度环Kp
extern float angle_Ki; // 角度环Ki
extern float angle_Kd; // 角度环Kd
extern float differ_Kp1[10]; // 偏差环Kp1
extern float differ_Kp2[10]; // 偏差环Kp2
extern float differ_Kp3[10]; // 偏差环Kp3
extern float differ_Kd[10]; // 偏差环Kd

/****************************** direction pid ******************************/
void dir_inner_init(void); // 初始化转向环PID
void dir_inner_pid(void); // 转向环PID
void differ_outer_init(void); // 初始化偏差环PID
void differ_outer_pid(void); // 偏差环PID
/****************************** gyroscope ******************************/
void gyroscope_init(void); // 初始化陀螺仪
void gyroscope_get_data(void); // 获取陀螺仪数据
void gyroscope_update(float gx, float gy, float gz, float ax, float ay, float az); // 四元数解算
void gyro_get_angle(void); // 获取陀螺仪角度
void gyroscope_deal_element(void); // 处理陀螺仪数据
float fast_sqrt(float x); // 快速平方根

#endif
