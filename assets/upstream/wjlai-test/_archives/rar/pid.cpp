#include "pid.h"
extern zf_driver_gpio  drv8701e_dir_1;
extern zf_driver_gpio  drv8701e_dir_2;
extern zf_driver_pwm   drv8701e_pwm_1;
extern zf_driver_pwm   drv8701e_pwm_2;

extern float filtered_left;
extern float filtered_right;


PID Left_motor;
PID Right_motor;
PID Position;
PID Gyro_PID;



void Position_Init(PID *PID, float p, float i, float d, float maxOutput)
{
	PID->kp = p;
	PID->ki = i;
	PID->kd = d;
	PID->maxOutput = maxOutput;
    PID->last_error = 0;
    PID->prev_error = 0;
    PID->output = 0;
}

void Gyro_PID_Init(PID *PID, float p, float i, float d, float maxOutput)
{
	PID->kp = p;
	PID->ki = i;
	PID->kd = d;
	PID->maxOutput = maxOutput;
    PID->last_error = 0;
    PID->prev_error = 0;
    PID->output = 0;
}

void Incremental_Motor_Init(PID *PID, float p, float i, float d, float maxOutput)
{
	PID->kp = p;
	PID->ki = i;
	PID->kd = d;
	PID->maxOutput = maxOutput;
    PID->last_error = 0;
    PID->prev_error = 0;
    PID->output = 0;
}


//方向环
float Positional_PID_Calc (PID *pid,float set_value, float get_value)
{
	float dout,pout;
    //更新数据
    pid->last_error = pid->error; 							//将旧error存起来
    pid->error = set_value - get_value; 					//计算新error

    //计算微分
    dout = (pid->error - pid->last_error) * pid->kd;
    //计算比例
    pout = pid->error * pid->kp;


    //计算输出
    pid->output = pout + dout + pid->integral;
    //输出限幅
    if (pid->output > pid->maxOutput)
        pid->output = pid->maxOutput;
    else if (pid->output < -pid->maxOutput)
        pid->output = -pid->maxOutput;
    
    return pid->output;
}

//角速度环
float Gyro_PID_Calc(PID *pid,float set_value, float get_value)
{
    float dout,pout;
    //更新数据
    pid->last_error = pid->error; 							//将旧error存起来
    pid->error = set_value - get_value; 					//计算新error

    //计算微分
    dout = (pid->error - pid->last_error) * pid->kd;
    //计算比例
    pout = pid->error * pid->kp;


    //计算输出
    pid->output = pout + dout + pid->integral;
    //输出限幅
    if (pid->output > pid->maxOutput)
        pid->output = pid->maxOutput;
    else if (pid->output < -pid->maxOutput)
        pid->output = -pid->maxOutput;
    
    return pid->output;
}


//速度环
float Incremental_Motor_Calc(PID *PID,float get_value)
{
    float error = PID->target_speed - get_value;          // 当前误差
    float increment = PID->kp * (error - PID->last_error)
                    + PID->ki * error
                    + PID->kd * (error - 2*PID->last_error + PID->prev_error);
    
    PID->output += increment;                   
    

    if (PID->output > PID->maxOutput) PID->output = PID->maxOutput;
    if (PID->output < -PID->maxOutput) PID->output = -PID->maxOutput;
    
    PID->prev_error = PID->last_error;
    PID->last_error = error;
    
    return PID->output;
}

int l_duty=0;
int r_duty=0;
void Motor_Control(float Speed_L,float Speed_R)
{
    Left_motor.target_speed  = Speed_L;
    Right_motor.target_speed = Speed_R;

    float left_duty=Incremental_Motor_Calc(&Left_motor,filtered_left);
    float right_duty=Incremental_Motor_Calc(&Right_motor,filtered_right);
    
    int DEADBAND_L = 350; // 左轮死区值，根据实际测试微调
    int DEADBAND_R = 350; // 右轮死区值（通道受伤，给大一点能强行拉快它的下地响应）

    // 【左轮死区补偿】
    if(left_duty > 10.0f)
    {
        left_duty += DEADBAND_L;
        // 补偿后必须重新用 maxOutput 限幅，防止 PWM 寄存器溢出
        if(left_duty > Left_motor.maxOutput) left_duty = Left_motor.maxOutput;
    }
    else if(left_duty < -10.0f)
    {
        left_duty -= DEADBAND_L;
        if(left_duty < -Left_motor.maxOutput) left_duty = -Left_motor.maxOutput;
    }

    // 【右轮死区补偿】
    if(right_duty > 10.0f)
    {
        right_duty += DEADBAND_R;
        if(right_duty > Right_motor.maxOutput) right_duty = Right_motor.maxOutput;
    }
    else if(right_duty < -10.0f)
    {
        right_duty -= DEADBAND_R;
        if(right_duty < -Right_motor.maxOutput) right_duty = -Right_motor.maxOutput;
    }


    l_duty=(int)left_duty;
    r_duty=(int)right_duty;

    if(left_duty>0)
    {
        drv8701e_dir_1.set_level(1);
        drv8701e_pwm_1.set_duty((int)left_duty);
    }
    else
    {
        drv8701e_dir_1.set_level(0);
        drv8701e_pwm_1.set_duty((int)-left_duty);
    }
    
    if(right_duty>0)
    {
        drv8701e_dir_2.set_level(1);
        drv8701e_pwm_2.set_duty((int)right_duty);
    }
    
    else
    {
        drv8701e_dir_2.set_level(0);
        drv8701e_pwm_2.set_duty((int)-right_duty);
    }

}

