#ifndef _SPEED_CONTROL_H_
#define _SPEED_CONTROL_H_

#include "zf_common_headfile.hpp"

/****************************** motor control ******************************/
#define  SYSTEM_ACC_TIME_MAX        (150)
#define  OVERLOAD_PWM_THRESHOLD     (3000)  // 过载PWM阈值
#define  OVERLOAD_SPEED_THRESHOLD   (50)    // 过载速度阈值
/****************************** filter ******************************/
#define  FILTER_SIZE                (4)     // 滤波窗口大小
/****************************** PID ******************************/
#define  ERROR_INTEGRAL             (40)    
/****************************** model speed ******************************/
#define  DETECT_DECREASE_SPEED      (0.5f)  // 红色色块减速速度系数
#define  TURN_SPEED                 (0.9f)  // 转弯速度系数
#define  TURN_DIFF_BOOST            (1.2f)  // 转弯差速放大系数
/****************************** BLDC control ******************************/
#define  USE_BLDC                   (0)     // 是否使用BLDC控制
#define  BLDC_MIN_PWM               (500)   // BLDC最小PWM
#define  BLDC_MAX_PWM               (1000)  // BLDC最大PWM
#define  BLDC_START_WAIT_TIME       (1000)  // BLDC起动等待时间(ms)
/****************************** speed decision ******************************/
#define  USE_SPEED_DECISION         (0)
#define  SPEED_DEAD_ZONE            (8.0)   // 直道误差死区
#define  STRAIGHT_SPEED             (1.4)   // 直道最大速度
#define  RAMP_DECREASE_SPEED        (0.8)   // 坡道最大速度 0.8 * set_speed
#define  MAX_SPEED_COEFFICIENT      (1.2)   // 最大速度系数
#define  MIN_SPEED_COEFFICIENT      (0.9)   // 最小速度系数
#define  INCREASE_MAX_SPEED         (3)     // 最大增加速度 3
#define  DECREASE_MAX_SPEED         (6)     // 最大减少速度 6

// encoder
struct encoder_struct // 编码器结构体
{
    int16_t left;
    int16_t right;
};
extern encoder_struct encoder;
// motor
extern zf_driver_gpio  drv8701e_dir_1; // 电机1方向引脚
extern zf_driver_gpio  drv8701e_dir_2; // 电机2方向引脚
extern zf_driver_pwm   drv8701e_pwm_1; // 电机1PWM引脚
extern zf_driver_pwm   drv8701e_pwm_2; // 电机2PWM引脚
extern int16_t set_speed;
extern int16_t constant_speed;
struct departure_struct // 发车结构体
{
    int16_t start_pwm;
    int16_t limit_start_pwm;
    int16_t pwm_max;
    int16_t limit_pwm_max;
};
extern departure_struct departure;
struct motor_stop // 电机停止结构体
{
    uint8_t stop_flag;
    uint8_t finish_stop_flag;
    bool detect_RGB_red;
    bool misjudge_flag;
};
extern motor_stop stop;
extern uint8_t integral_limit_flag;
struct model_control_speed // 模型控制速度结构体
{
    uint8_t block_x;
    uint8_t block_y;
    uint8_t middle_end_row;
    int8_t middle_end_column;
    bool turn_flag;
    bool return_flag;
};
extern model_control_speed model_control;
// INC PID
struct PID_Inc // 增量式PID
{
    float Kp;                       // Kp
    float Ki;                       // Ki
    float Kd;                       // Kd
    float pin;                      // 前馈
    float set;                      // 设定值
    float last_set;                 // 上一次设定值
    float actual;                   // 真实值
    float err;                      // 偏差值
    float last_err;                 // 上次偏差值
    float llast_err;                // 上上次偏差值
    float increment;                // PID增量
    float proportion;               // 比例
    float integral;                 // 积分
    float differential;             // 微分
    float limit_integral;           // 积分限幅
    float limit_actual_out;         // 输出限幅
    float out;                      // PID输出
    float actual_out;               // 实际输出
};
extern PID_Inc right_speed, left_speed;
// filter
struct moving_filter // 滑动滤波结构体
{
    int16_t buffer[FILTER_SIZE];    // 数据缓冲区
    uint8_t index;				    // 数组索引
    int32_t sum;				    // 缓冲区数据之和
};
extern moving_filter filter_left; // 左边电机滑动滤波
extern moving_filter filter_right; // 右边电机滑动滤波
struct low_pass_filter // 低通滤波结构体
{
    float last_prev;   // 上一次输出值
};
extern low_pass_filter gyro_yaw_filter;
// BLDC control
struct BLDC_control_struct // BLDC控制结构体
{
    bool start_lock;
    uint16_t bldc_constant_pwm;
};
extern BLDC_control_struct BLDC_control;
// speed decision
struct speed_decision_struct // 速度决策结构体
{
    int16_t target_speed;
    int16_t last_speed;
    int16_t max_speed;
    int16_t min_speed;
    float turn_angle;
    float turn_angle_max;
};
extern speed_decision_struct decision;

/****************************** motor ******************************/
void motor_init(void); // 电机初始化
void speed_control(void); // 速度控制
void motor_departure(int16_t max_acc_pwm); // 发车限幅
void speed_calculate_dir(void); // 计算速度大小和方向
void motor_control(float out_max); // 电机控制
void prevent_motor_rotation(void); // 防止电机疯转
void motor_control_stop(void); // 电机控制停止
/****************************** PID ******************************/
void pid_inc_init(void); // PID初始化
void PID_increment(PID_Inc *PID); // PID增量计算
/****************************** filter ******************************/
void moving_filter_init(moving_filter* filter, int16_t initial_value); // 滑动滤波初始化
int16_t moving_filter_calculate(moving_filter* filter, int16_t refresh_value); // 滑动滤波计算
float lowpass_filter(low_pass_filter* flt, float input, float alpha); // 低通滤波计算
/****************************** speed decision ******************************/
int16_t speed_decision(void); // 速度决策

#endif
