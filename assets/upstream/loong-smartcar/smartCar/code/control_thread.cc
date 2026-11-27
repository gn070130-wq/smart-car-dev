#include "control_thread.h"

//--控制对象--
ICM20602 imc20602;
Motor_fans fans;
Motor motor;
Servo servo;

//智能车速度
int smartCar_speed = 0;
//电机使能
bool motor_enable = false;
//负压使能
bool fans_enable = false;
//控制线程
thread ctrl_thread;

//控制锁
mutex control_mutex;
//电机使能锁
mutex motor_enable_mutex;
//电机启动倒计时锁
mutex motor_enable_enable_mutex;
//电机启动倒计时
int motor_enable_time = 0;
//使能电机使能
bool motor_enable_enable = false;
//陀螺仪z、y轴积分
float gyro_z_integration = 0;
float gyro_y_integration = 0;
//编码器积分
int motor_distance = 0;

//负压激增
int fans_time_duty = 0;

//陀螺仪抑制后舵机输出
float Servo_Ctrl = 0;

//跟踪距离
int trackAim = 0;

//编码器积分
void motor_distance_control(void)
{
    motor_distance += (motor.LencoderCount + motor.RencoderCount) / 2;
}

//清除编码器积分
void motor_distance_cleam(void)
{
    motor_distance = 0;
}

//陀螺仪积分
void gyro_integration_control(int ms)
{
    if(imc20602.gyro_z != 0)
        gyro_z_integration += (imc20602.gyro_z / (1000 / ms) + 0.00058) * 1.24137;
    if(imc20602.gyro_y != 0)
        gyro_y_integration += (imc20602.gyro_y / (1000 / ms) - 0.00175) * 1.24137;
    
}

//清除陀螺仪积分
void gyro_integration_clean(void)
{
    gyro_z_integration = 0;
    gyro_y_integration = 0;
}

//控制决策声明
void control_decision(void);
//使能电机使能控制器声明
void motor_enable_enable_control(void);

void controlThraed(void)
{
    
    while(1) {
        usleep(10000);
        //大津法重装
        ReOTSU_Enable_timer(100);
        //使能电机使能控制器
        motor_enable_enable_control();
        //陀螺仪角速度读取
        imc20602.upDataGyro();
        //陀螺仪积分
        gyro_integration_control(10);

        //控制上锁
        control_mutex.lock();
        //模糊控制器
        fuzzy.get_updataPD_pid(ImgErr, ImgErr-servo_pid.error, &servo_pid.kp, &servo_pid.kd);
        //位置式pid控制
        Positional_PID_Calc(&servo_pid, 0, ImgErr);
        //控制解锁
        control_mutex.unlock();

        //控制决策
        control_decision();
        //电机实时速度获取
        motor.getSpeed();
        //编码器积分
        motor_distance_control();
        //舵机电机控制
        servo.set_servo(Servo_Ctrl);
        motor.setSpeed(motor.Ltarget, motor.Rtarget);
        fans.fans_set_duty(fans.dutyL, fans.dutyR);
    }
}

void controlThread_Star(void)
{
    ctrl_thread = thread(controlThraed);
    ctrl_thread.detach();
}

//速度决策声明
void speed_decision(void);
//阿克曼控制声明
void ackerman_control(void);
//负压控制
void fans_control(int duty);


//控制决策实现
void control_decision(void)
{
    //速度+控制决策
    speed_decision();
    //陀螺仪抑制后的舵机最后值
    Servo_Ctrl = servo_pid.output - imc20602.gyro_z * preprocess.Gryo_Zkd;
    //获取转弯半径（第二个传参为阈值）
    get_turningRadius(Servo_Ctrl, preprocess.akmThreshold);
    //阿克曼后轮控制（电机未使能目标速度为0）
    ackerman_control();
    //负压控制
    if(fans_enable) {
        fans_time_duty = (fans_time_duty + 1 > preprocess.fans_Maxduty) ? preprocess.fans_Maxduty : fans_time_duty + 1;

        fans_control(fans_time_duty);
    }
    else if(!fans_enable) {
        fans_time_duty = 0;
        fans_control(0);
    }
}

//阿克曼控制实现
void ackerman_control(void)
{
    if(!motor_enable){
        motor.Ltarget = 0;
        motor.Rtarget = 0;
        Servo_Ctrl = 0;
        return ;
    }
    //有转弯半径才进行阿克曼控制
    if(turningRadius > 0) 
    {
        motor.Ltarget = ackerman_Lmotor(smartCar_speed, preprocess.ICAR_HALFW + preprocess.LEFT_HALFW_BUF);
        motor.Rtarget = ackerman_Rmotor(smartCar_speed, preprocess.ICAR_HALFW + preprocess.LEFT_HALFW_BUF);
    }
    else if(turningRadius < 0)
    {
        motor.Ltarget = ackerman_Lmotor(smartCar_speed, preprocess.ICAR_HALFW);
        motor.Rtarget = ackerman_Rmotor(smartCar_speed, preprocess.ICAR_HALFW);
    } 
    else 
    {
        motor.Ltarget = smartCar_speed;
        motor.Rtarget = smartCar_speed;
    }
    
}

void fans_control(int duty)
{
    fans.dutyL = duty;
    fans.dutyR = duty;
}

//速度+控制决策实现
void speed_decision(void)
{
    switch (roadstate)
    {
    //常规路段
    case Normal_Road:
    {
        switch (normalstate)
        {
        //常规直道路
        case Normal_longway:
            servo_pid.output = servo_pid.output * preprocess.long_restrain;
            smartCar_speed = preprocess.motorspeed_high;
            break;
        //常规弯道
        case Normal_curve:
            smartCar_speed = preprocess.motorspeed_low;
            break;
        //特殊直道
        case Normal_longway_weak:
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            smartCar_speed = preprocess.motorspeed_weakhigh;
            break;
        default:
            break;
        }
        break;
    }

    //十字路段
    case Cross_Road:
    {
        switch (cross_state)
        {
        
        case front_crossRoad_1:
            smartCar_speed = preprocess.crossSpeed_h;
            servo_pid.output = servo_pid.output * preprocess.long_restrain;
            break;
        break;

        case front_crossRoad_2:
            smartCar_speed = preprocess.crossSpeed_h;
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            break;
        break;

        case front_crossRoad_3:
            smartCar_speed = preprocess.crossSpeed_h;
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            break;
        break;

        case in_crossRoad_1:
            smartCar_speed = preprocess.crossSpeed_h;
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            break;
        
        default:
            smartCar_speed = preprocess.crossSpeed_l;
            break;
        }
        break;
    }
    //圆环路段
    case Ring_Road:
    {
        switch (ringstate)
        {
        case Right_intoRing:
            smartCar_speed = preprocess.ring_intoSpeed;
            break;

        case Right_inRing:
            smartCar_speed = preprocess.ringSpeed;
            break;

        case Right_ReadyoutRing:
            smartCar_speed = preprocess.ringSpeed;
            break;

        case Right_outRing:
            smartCar_speed = preprocess.ringSpeed;
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            break;

        case Left_intoRing:
            smartCar_speed = preprocess.ring_intoSpeed;
            break;

        case Left_inRing:
            smartCar_speed = preprocess.ringSpeed;
            break;

        case Left_ReadyoutRing:
            smartCar_speed = preprocess.ringSpeed;
            break;

        case Left_outRing:
            smartCar_speed = preprocess.ringSpeed;
            servo_pid.output = servo_pid.output * preprocess.long_weak_restrain;
            break;
        
        default:
            break;
        }
        break;
    }

    case Barrier_Road:
    {
        servo_pid.output = servo_pid.output * preprocess.barrier_buffer;
        smartCar_speed = preprocess.barrier_speed;
        break;
    }

    case Bridge_Road:
    {
        switch (bridge_state)
        {
        case Bridge_up:
            servo_pid.output = servo_pid.output * 0.5;
            smartCar_speed = preprocess.bridge_speed;
            break;

        case Bridge_down:
            servo_pid.output = servo_pid.output * 0.5;
            smartCar_speed = preprocess.bridge_speed;
            break;

        case Bridge_out:
            servo_pid.output = servo_pid.output * 0.5;
            smartCar_speed = preprocess.bridge_speed;
            break;
        
        default:
            break;
        }
        
        break;
    }
    
    default:
        break;
    }
}

//电机失能（直接失能)
void motor_enable_off(void)
{
    if(!motor_enable) return ;
    //关闭电机使能
    motor_enable_mutex.lock();
    motor_enable = false;
    motor_enable_mutex.unlock();
}

//电机使能（直接使能)
void motor_enable_on(void)
{
    if(motor_enable) return ;
    //开启电机使能
    motor_enable_mutex.lock();
    motor_enable = true;
    motor_enable_mutex.unlock();
}

//使能电机使能控制实现
void motor_enable_enable_control(void)
{
    //如果使能电机使能为false不启用
    if(!motor_enable_enable) return ;
    //如果使能电机使能为true 使能电机使能计数器>0
    if(motor_enable_enable && motor_enable_time>0)
    {
        motor_enable_enable_mutex.lock();
        motor_enable_time--;
        motor_enable_enable_mutex.unlock();
    }
    //如果使能电机使能为true 使能电机使能计数器==0
    else if(motor_enable_enable && motor_enable_time == 0)
    {
        motor_enable_enable_mutex.lock();
        motor_enable_enable = false;
        motor_enable_enable_mutex.unlock();
        motor_enable_on();
    }
}

//使能电机使能 ms:倒计时
void motor_enable_star(int ms)
{
    motor_enable_enable_mutex.lock();
    motor_enable_time = ms / 10;
    motor_enable_enable = true;
    motor_enable_enable_mutex.unlock();
}