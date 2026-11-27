#include "chassis.hpp"
#include <cstring>

#define ENCODER_PATH_LEFT                      ZF_ENCODER_QUAD_1          
#define ENCODER_PATH_RIGHT                     ZF_ENCODER_QUAD_2

#define Motor_PATH_Left                       ZF_PWM_MOTOR_1
#define DIR_LEFT_PATH                         ZF_GPIO_MOTOR_1

#define Motor_PATH_Right                       ZF_PWM_MOTOR_2
#define DIR_RIGHT_PATH                      ZF_GPIO_MOTOR_2           

#define MOTOR_LEFT_DIR_LEVEL           1       //左电机方向
#define MOTOR_RIGHT_DIR_LEVEL          1       //右电机方向

#define ENCODER_LEFT_SIGN              1       //编码器计数符号，正数表示正向，负数表示反向，根据实际安装调整
#define ENCODER_RIGHT_SIGN             -1       //编码器计数符号，正数表示正向，负数表示反向，根据实际安装调整


// PWM占空比上限,在设备树中，设置的10000。如果要修改，需要与设备树对应
#define MOTOR_LEFT_PWM_DUTY_MAX    (drv8701e_pwm_left_info.duty_max)       
#define MOTOR_RIGHT_PWM_DUTY_MAX    (drv8701e_pwm_right_info.duty_max) 

#define MAX_PWM_DUTY        (90)   //最大 MAX_PWM_DUTY% 占空比

#define SPEED_LEFT_KP                      0.35f
#define SPEED_LEFT_KI                      0.01f
#define SPEED_LEFT_KD                      0.0f

#define SPEED_RIGTH_KP                     0.35f
#define SPEED_RIGTH_KI                     0.012f
#define SPEED_RIGTH_KD                     0.0f

#define VISION_KP                          280.0f
#define VISION_KD                          0.0f
#define VISION_KP_HEAD_ERROR               230.0f
#define VISION_KD_HEAD_ERROR               0.0f
#define GYRO_KP                            0.10f
#define GYRO_KD                            0.0f
#define MAX_GYRO_Z                         8000.0f
#define TARGET_GYRO_Z_STEP_MAX             4000.0f  //限制视觉外环目标角速度单帧突变，避免红色绕行瞬间急打方向
#define BASE_SPEED                          350     //基准速度，单位为编码器计数每周期
#define RED_AVOIDANCE_SPEED                 90     //检测到红色物块后的绕行速度
#define BASE_SPEED_RAMP_STEP                2.0f    //每5ms增加的基准速度，450约0.5s升到目标
#define BASE_SPEED_DECEL_STEP               25.0f   //等待识别时每5ms快速降低的基准速度
#define CHASSIS_WAVEFORM_SEND_DIV           1       // 波形发送分频，10ms控制周期下每50ms发一次

#define ASSISTANT_CHANNEL_VISION_KP        1
#define ASSISTANT_CHANNEL_VISION_KD        2
#define ASSISTANT_CHANNEL_VISION_KP_HEAD   3
#define ASSISTANT_CHANNEL_VISION_KD_HEAD   4
#define ASSISTANT_CHANNEL_BASE_SPEED       5
#define ASSISTANT_CHANNEL_GYRO_KP          6
#define ASSISTANT_CHANNEL_GYRO_KD          7

#define LSB 16.4f

      
//===========================================================================速度内环变量定义===========================================================================//
float Target_Speed_Left,Target_Speed_Right,Actual_Speed_Left,Actual_Speed_Right,Out_Pwm_Left,Out_Pwm_Right;                    //实际速度，目标速度
float Speed_Error0_Left,Speed_Error1_Left,Speed_Error2_Left,Speed_Error0_Right,Speed_Error1_Right,Speed_Error2_Right;         //本次误差，上次误差，上上次误差                                      
float Kp_Left = SPEED_LEFT_KP, Ki_Left = SPEED_LEFT_KI, Kd_Left = SPEED_LEFT_KD;                                              //左轮PID参数
float Kp_Right = SPEED_RIGTH_KP, Ki_Right = SPEED_RIGTH_KI, Kd_Right = SPEED_RIGTH_KD;                                       //右轮PID参数

//==========================================================================视觉外环变量定义============================================================================//
float Vision_Error0,Vision_Error1,Vision_Error_Head1,Vision_Error_Head0;         //本次误差,上次误差
float Kp = VISION_KP, Kd = VISION_KD, Kp_Head = VISION_KP_HEAD_ERROR, Kd_Head = VISION_KD_HEAD_ERROR;  //视觉外环PD参数
float Base_Speed = BASE_SPEED;                            // 运行时基准速度，可由逐飞助手在线调整
static float Ramped_Base_Speed = 0.0f;                    // 实际交给速度环的软启动速度

//==========================================================================陀螺仪中环变量定义============================================================================//
float Target_Gyro_Z, Actual_Gyro_Z, Gyro_Error0, Gyro_Error1;   //目标角速度、实际角速度和误差
float Kp_Gyro = GYRO_KP, Kd_Gyro = GYRO_KD;                     //陀螺仪环PD参数
float Gyro_Correction_Speed;                                    //陀螺仪环输出的差速补偿值

struct  pwm_info drv8701e_pwm_left_info;
struct  pwm_info drv8701e_pwm_right_info;

zf_driver_encoder Encoder_Left(ENCODER_PATH_LEFT);
zf_driver_encoder Encoder_Right(ENCODER_PATH_RIGHT);

zf_driver_pwm Motor_Pwm_Left(Motor_PATH_Left);
zf_driver_pwm Motor_Pwm_Right(Motor_PATH_Right);

zf_driver_gpio Motor_Dir_Left(DIR_LEFT_PATH,O_RDWR);
zf_driver_gpio Motor_Dir_Right(DIR_RIGHT_PATH,O_RDWR);
zf_device_imu Imu;

int8  Duty_Left = 0;          //左电机PWM占空比
int8  Duty_Right = 0;         //右电机PWM占空比


float gyro_z_offset=0.0f; //陀螺仪Z轴偏移值
float Imu_Yaw = 0.0f; // 圆环状态机使用的偏航角积分值，单位约为度
static uint8 waveform_send_div_count = 0;   
static uint8 yaw_get_state = 0; // 圆环偏航角积分状态，0-清零并停止积分 1-开始积分
static uint8 last_red_avoidance_state = RED_AVOIDANCE_STATE_IDLE;
static int32 red_avoidance_encoder_sum = 0;

static void chassis_sync_vision_pid_to_assistant_channels(void)
{
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KP - 1] = Kp;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KD - 1] = Kd;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KP_HEAD - 1] = Kp_Head;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KD_HEAD - 1] = Kd_Head;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_BASE_SPEED - 1] = Base_Speed;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_GYRO_KP - 1] = Kp_Gyro;
    seekfree_assistant_parameter[ASSISTANT_CHANNEL_GYRO_KD - 1] = Kd_Gyro;
}

static void chassis_reset_speed_loop(void)
{
    Speed_Error0_Left = 0;
    Speed_Error1_Left = 0;
    Speed_Error2_Left = 0;
    Speed_Error0_Right = 0;
    Speed_Error1_Right = 0;
    Speed_Error2_Right = 0;
    Out_Pwm_Left = 0;
    Out_Pwm_Right = 0;
}



void chassis_init(void)
{
    //获取PWM设备信息
    Motor_Pwm_Left.get_dev_info(&drv8701e_pwm_left_info);
    Motor_Pwm_Right.get_dev_info(&drv8701e_pwm_right_info);

    //设置电机初始状态为停止
    Motor_Dir_Left.set_level(0);
    Motor_Pwm_Left.set_duty(0);
    Motor_Dir_Right.set_level(0);
    Motor_Pwm_Right.set_duty(0);

    //编码器计数清零
    Encoder_Left.clear_count();
    Encoder_Right.clear_count();

    //误差清零
    chassis_reset_speed_loop();
    Vision_Error0 = 0;
    Vision_Error1 = 0;
    Vision_Error_Head1=0;
    Vision_Error_Head0=0;
    Target_Gyro_Z = 0;
    Actual_Gyro_Z = 0;
    Gyro_Error0 = 0;
    Gyro_Error1 = 0;
    Gyro_Correction_Speed = 0;
    waveform_send_div_count = 0;
    Imu_Yaw = 0.0f;
    last_red_avoidance_state = RED_AVOIDANCE_STATE_IDLE;
    Imu.init();

    // 从0开始软启动，避免第一个控制周期将PWM顶到上限。
    Ramped_Base_Speed = 0.0f;
    Target_Speed_Left = 0.0f;
    Target_Speed_Right = 0.0f;

    // 逐飞助手参数通道映射:
    // 测试用逐飞助手参数通道映射:
    // 1-VISION_KP  2-VISION_KD  3-VISION_KP_HEAD_ERROR  4-VISION_KD_HEAD_ERROR  5-BASE_SPEED  6-GYRO_KP  7-GYRO_KD  8-reference_contrast_ratio
    chassis_sync_vision_pid_to_assistant_channels();
}

void chassis_stop(void)
{
    //设置电机停止
    Motor_Dir_Left.set_level(0);
    Motor_Pwm_Left.set_duty(0);
    Motor_Dir_Right.set_level(0);
    Motor_Pwm_Right.set_duty(0);

    //编码器计数清零
    Encoder_Left.clear_count();
    Encoder_Right.clear_count();

    //误差清零
    chassis_reset_speed_loop();
    Vision_Error0 = 0;
    Vision_Error1 = 0;
    Vision_Error_Head1 = 0;
    Vision_Error_Head0 = 0;
    Target_Gyro_Z = 0;
    Actual_Gyro_Z = 0;
    Gyro_Error0 = 0;
    Gyro_Error1 = 0;
    Gyro_Correction_Speed = 0;
    waveform_send_div_count = 0;
    last_red_avoidance_state = RED_AVOIDANCE_STATE_IDLE;
    Ramped_Base_Speed = 0.0f;
    Target_Speed_Left = 0.0f;
    Target_Speed_Right = 0.0f;

}
void chassis_vision_control_tick(VisionResult &vision_result)
{
    uint8 previous_red_avoidance_state = last_red_avoidance_state;
    if(RED_AVOIDANCE_STATE_IDLE == previous_red_avoidance_state &&
       RED_AVOIDANCE_STATE_IDLE != vision_result.red_avoidance_state)
    {
        chassis_reset_speed_loop();
        red_avoidance_encoder_sum = 0;
    }
    if(RED_AVOIDANCE_STATE_WAIT_DIRECTION == previous_red_avoidance_state &&
       (RED_AVOIDANCE_STATE_LEFT == vision_result.red_avoidance_state ||
        RED_AVOIDANCE_STATE_RIGHT == vision_result.red_avoidance_state ||
        RED_AVOIDANCE_STATE_STRAIGHT == vision_result.red_avoidance_state))
    {
        Ramped_Base_Speed = (Actual_Speed_Left + Actual_Speed_Right) / 2.0f;
    }
    if(RED_AVOIDANCE_STATE_RETURN_TRACK == vision_result.red_avoidance_state &&
       RED_AVOIDANCE_STATE_RETURN_TRACK != previous_red_avoidance_state)
    {
        Ramped_Base_Speed = Base_Speed;
    }
    last_red_avoidance_state = vision_result.red_avoidance_state;

   //----------------------------------------------视觉外环PD-----------------------------------------------//
    //获取本次误差和上次误差
    Vision_Error1 = Vision_Error0;
    Vision_Error0 = vision_result.line_error;

    //获取本次前瞻的误差和上次前瞻的误差
    Vision_Error_Head1 = Vision_Error_Head0;
    Vision_Error_Head0 = vision_result.heading_error;

    //视觉外环输出目标角速度，交给陀螺仪中环闭环
    float target_gyro_z = Kp * Vision_Error0
                        + Kd * (Vision_Error0 - Vision_Error1)
                        + Kp_Head * Vision_Error_Head0
                        + Kd_Head * (Vision_Error_Head0 - Vision_Error_Head1);

    if(target_gyro_z > MAX_GYRO_Z)
        target_gyro_z = MAX_GYRO_Z;
    if(target_gyro_z < -MAX_GYRO_Z)
        target_gyro_z = -MAX_GYRO_Z;

    float target_gyro_step = target_gyro_z - Target_Gyro_Z;
    if(target_gyro_step > TARGET_GYRO_Z_STEP_MAX)
        target_gyro_step = TARGET_GYRO_Z_STEP_MAX;
    if(target_gyro_step < -TARGET_GYRO_Z_STEP_MAX)
        target_gyro_step = -TARGET_GYRO_Z_STEP_MAX;
    Target_Gyro_Z += target_gyro_step;
}

void chassis_gyro_control_tick(void)
{
   //----------------------------------------------陀螺仪中环PD-----------------------------------------------//
    // Target_Gyro_Z 由视觉外环或测试代码设置，这里只做角速度闭环。
    Actual_Gyro_Z = -(Imu.get_gyro_z() - gyro_z_offset);

    Gyro_Error1 = Gyro_Error0;
    Gyro_Error0 = Target_Gyro_Z - Actual_Gyro_Z;

    Gyro_Correction_Speed = Kp_Gyro * Gyro_Error0
                          + Kd_Gyro * (Gyro_Error0 - Gyro_Error1);

    uint8_t red_avoidance_state = last_red_avoidance_state;
    float target_base_speed = (RED_AVOIDANCE_STATE_WAIT_DIRECTION == red_avoidance_state) ?
                              RED_AVOIDANCE_SPEED : Base_Speed;
    if(Ramped_Base_Speed < target_base_speed)
    {
        Ramped_Base_Speed += BASE_SPEED_RAMP_STEP;
        if(Ramped_Base_Speed > target_base_speed)
            Ramped_Base_Speed = target_base_speed;
    }
    else if(Ramped_Base_Speed > target_base_speed)
    {
        Ramped_Base_Speed -= BASE_SPEED_DECEL_STEP;
        if(Ramped_Base_Speed < target_base_speed)
            Ramped_Base_Speed = target_base_speed;
    }

    Target_Speed_Left = Ramped_Base_Speed;
    Target_Speed_Right = Ramped_Base_Speed;
    if(Gyro_Correction_Speed > 0)
        Target_Speed_Right -= Gyro_Correction_Speed;
    else
        Target_Speed_Left += Gyro_Correction_Speed;

    // 圆环状态机用偏航角：对Z轴角速度积分，仅在 yaw_start_cout(1) 后累计
    if(yaw_get_state == 1)
    {
        Imu_Yaw += Actual_Gyro_Z / LSB * 0.005f;
    }
    else
    {
        Imu_Yaw = 0;
    }
}

void chassis_speed_control_tick(void)   //底盘速度控制周期函数，每5ms调控一次
{
    

    //----------------------------------------------速度内环PID-----------------------------------------------//
    //获取实际值
    int32 encoder_count_left = Encoder_Left.get_count() * ENCODER_LEFT_SIGN;
    Actual_Speed_Left=encoder_count_left;
    Encoder_Left.clear_count();
    
    int32 encoder_count_right = Encoder_Right.get_count() * ENCODER_RIGHT_SIGN;
    Actual_Speed_Right=encoder_count_right;
    Encoder_Right.clear_count();

    if(RED_AVOIDANCE_STATE_LEFT == last_red_avoidance_state ||
       RED_AVOIDANCE_STATE_RIGHT == last_red_avoidance_state ||
       RED_AVOIDANCE_STATE_STRAIGHT == last_red_avoidance_state)
    {
        red_avoidance_encoder_sum += encoder_count_left + encoder_count_right;
    }

    //获取本次误差，上次误差和上上次误差
    Speed_Error2_Left=Speed_Error1_Left;
    Speed_Error1_Left=Speed_Error0_Left;
    Speed_Error0_Left=Target_Speed_Left-Actual_Speed_Left;

    Speed_Error2_Right=Speed_Error1_Right;
    Speed_Error1_Right=Speed_Error0_Right;
    Speed_Error0_Right=Target_Speed_Right-Actual_Speed_Right;

    //PID计算
    Out_Pwm_Left += Kp_Left*(Speed_Error0_Left-Speed_Error1_Left)+Ki_Left*Speed_Error0_Left+Kd_Left*(Speed_Error0_Left-2*Speed_Error1_Left+Speed_Error2_Left);
    Out_Pwm_Right+= Kp_Right*(Speed_Error0_Right-Speed_Error1_Right)+Ki_Right*Speed_Error0_Right+Kd_Right*(Speed_Error0_Right-2*Speed_Error1_Right+Speed_Error2_Right);

    //输出限幅
    if(Out_Pwm_Left > MAX_PWM_DUTY)
        Out_Pwm_Left = MAX_PWM_DUTY;
    if(Out_Pwm_Left < -MAX_PWM_DUTY)
        Out_Pwm_Left = -MAX_PWM_DUTY;
    if(Out_Pwm_Right > MAX_PWM_DUTY)        
        Out_Pwm_Right = MAX_PWM_DUTY;
    if(Out_Pwm_Right < -MAX_PWM_DUTY)
        Out_Pwm_Right = -MAX_PWM_DUTY;

    //设置PWM占空比，正负值表示转动方向
    if(Out_Pwm_Left >= 0)
    {
        Motor_Dir_Left.set_level(MOTOR_LEFT_DIR_LEVEL);   //设置方向
        Motor_Pwm_Left.set_duty(Out_Pwm_Left* (MOTOR_LEFT_PWM_DUTY_MAX / 100));           //设置占空比
    }
    else
    {
        Motor_Dir_Left.set_level(!MOTOR_LEFT_DIR_LEVEL);  //设置方向
        Motor_Pwm_Left.set_duty(-Out_Pwm_Left* (MOTOR_LEFT_PWM_DUTY_MAX / 100));          //设置占空比
    }

    if(Out_Pwm_Right >=0)
    {
        Motor_Dir_Right.set_level(MOTOR_RIGHT_DIR_LEVEL);   //设置方向
        Motor_Pwm_Right.set_duty(Out_Pwm_Right* (MOTOR_RIGHT_PWM_DUTY_MAX / 100));           //设置占空比
    }
    else
    {
        Motor_Dir_Right.set_level(!MOTOR_RIGHT_DIR_LEVEL);  //设置方向
        Motor_Pwm_Right.set_duty(-Out_Pwm_Right* (MOTOR_RIGHT_PWM_DUTY_MAX / 100));          //设置占空比
    }
}

void yaw_offset_calibration(void)
{
    const uint16_t sample_count = 100;
     int32 sum = 0;
    for(uint16_t i = 0; i < sample_count; i++)
    {
        sum += Imu.get_gyro_z();
        system_delay_ms(5); // 每5ms采样一次，持续约0.5秒
    }
    gyro_z_offset = (float)sum / sample_count;
}

bool chassis_red_avoidance_encoder_reached(int32 target_count)
{
    return (RED_AVOIDANCE_STATE_LEFT == last_red_avoidance_state ||
            RED_AVOIDANCE_STATE_RIGHT == last_red_avoidance_state ||
            RED_AVOIDANCE_STATE_STRAIGHT == last_red_avoidance_state) &&
           red_avoidance_encoder_sum / 2 >= target_count;
}

void chassis_send_debug_waveform(const VisionResult &vision_result) //逐飞助手发送示波器波形数据函数
{
    waveform_send_div_count++;
    if(waveform_send_div_count < CHASSIS_WAVEFORM_SEND_DIV)
    {
        return;
    }
    waveform_send_div_count = 0;

    seekfree_assistant_oscilloscope_data.channel_num = 8;
    seekfree_assistant_oscilloscope_data.data[0] = Target_Speed_Left;
    seekfree_assistant_oscilloscope_data.data[1] = Actual_Speed_Left;
    seekfree_assistant_oscilloscope_data.data[2] = Target_Speed_Right;
    seekfree_assistant_oscilloscope_data.data[3] = Actual_Speed_Right;
    seekfree_assistant_oscilloscope_data.data[4] = vision_result.red_avoidance_state * 100; // 0-空闲 100-等待方向 200-左绕 300-右绕 400-直行 500-回归赛道
    seekfree_assistant_oscilloscope_data.data[5] = vision_result.red_found ?
                                                  vision_result.red_y + vision_result.red_h : -1; // 红块底边行号，-1 为未识别
    seekfree_assistant_oscilloscope_data.data[6] = Target_Gyro_Z;
    seekfree_assistant_oscilloscope_data.data[7] = Gyro_Correction_Speed;

    seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);
}

void chassis_update_vision_pid_from_assistant(void)
{
    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KP - 1])
    {
        Kp = seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KP - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KP - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KD - 1])
    {
        Kd = seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KD - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KD - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KP_HEAD - 1])
    {
        Kp_Head = seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KP_HEAD - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KP_HEAD - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KD_HEAD - 1])
    {
        Kd_Head = seekfree_assistant_parameter[ASSISTANT_CHANNEL_VISION_KD_HEAD - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_VISION_KD_HEAD - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_BASE_SPEED - 1])
    {
        Base_Speed = seekfree_assistant_parameter[ASSISTANT_CHANNEL_BASE_SPEED - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_BASE_SPEED - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_GYRO_KP - 1])
    {
        Kp_Gyro = seekfree_assistant_parameter[ASSISTANT_CHANNEL_GYRO_KP - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_GYRO_KP - 1] = 0;
    }

    if(seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_GYRO_KD - 1])
    {
        Kd_Gyro = seekfree_assistant_parameter[ASSISTANT_CHANNEL_GYRO_KD - 1];
        seekfree_assistant_parameter_update_flag[ASSISTANT_CHANNEL_GYRO_KD - 1] = 0;
    }
}

float yaw_get_from_imu(void) // 获取圆环状态机使用的偏航角积分值
{
    return -Imu_Yaw;
}

void yaw_start_cout(uint8_t state) // 控制圆环偏航角是否开始积分
{
    if(state == 0)
    {
        yaw_get_state = 0;
    }
    else
    {
        yaw_get_state = 1;
    }
}
