#include "isr.hpp"
#include "ww_camera_server.h"
#include "menu.hpp"
zf_driver_pit pit_timer;
int isr_cnt=0;
uint32 run_time_cnt = 0; // 【新增】：专门用于记录小车起步后的运行时间
int pwm_N=850;
int temp_run_flag=0;
static std::atomic<bool> runtime_reset_guard(false);
static std::atomic<bool> isr1_active(false);

void begin_runtime_reset_guard()
{
    runtime_reset_guard.store(true, std::memory_order_seq_cst);
    while(isr1_active.load(std::memory_order_seq_cst))
    {
        std::this_thread::yield();
    }
}

void end_runtime_reset_guard()
{
    runtime_reset_guard.store(false, std::memory_order_seq_cst);
}

void reset_isr_runtime_state()
{
    isr_cnt = 0;
    run_time_cnt = 0;
    temp_run_flag = 0;
}

void Pit_pid_test(){

       const float channels[] = {
        (float)speed_left_target,(float)speed_right_target,(float)speed_left_now,(float)speed_right_now,(float)Dir_output,(float)Gyro_output,(float)car_running,
        (float)speed_left_output,(float)speed_right_output,(float)col_Differ,(float)Gyro_Feedforward,(float)pre_brake
    }; 
    upload_data_channels("pid", channels, sizeof(channels) / sizeof(channels[0]));
}
void isr1_handler()
{
    if(runtime_reset_guard.load(std::memory_order_seq_cst)) return;
    isr1_active.store(true, std::memory_order_seq_cst);
    if(runtime_reset_guard.load(std::memory_order_seq_cst))
    {
        isr1_active.store(false, std::memory_order_seq_cst);
        return;
    }

    // Motor_pwm_set(1000,-1000);
    // pwm_fan.set_duty(10000);
    // PWM_FAN_SET(9000);
    // run_time_cnt++;//每2ms增加一次，500ms为250次，1000ms为500次，1500ms为750次
    
    // if(run_time_cnt>500)
    // {
    //     PWM_FAN_SET(run_time_cnt);
        
    // }
    // if(run_time_cnt>pwm_N)run_time_cnt=pwm_N;

    // if(run_time_cnt>250&&run_time_cnt<=1000)//500ms-1000ms
    // {
    //    PWM_FAN_SET(650);
    // }
    // else if(run_time_cnt>1000&&run_time_cnt<=1500)//1000ms-1500ms
    // {
    //     PWM_FAN_SET(800);
    // }
    // else if(run_time_cnt>1500)//1500ms后车辆启动
    // {
    //     PWM_FAN_SET(900);
    // }
    // speed_mensure();//测速
    // speed_close_PID(0,0);//速度环目标速度为0，停车
    Pit_pid_test();
    isr_cnt++;
    gyroz_read();//读取陀螺仪
    gyro_integrate();
    speed_mensure();//测速
    update_card_stop_feedback();//卡片停车阶段：连续确认左右轮实际速度已经归零
    update_card_reverse_feedback();//倒车到距后：连续确认两轮已经再次停稳
    update_card_recognition_cooldown();//卡片成功分类后的时间冷却，2ms递减一次
    // Get_DIS();//测距
    Distance_interial();//距离积分
    if(isr_cnt%4==0) Dir_PID();//8ms一次方向环
    // Dir_output=0;
    if(isr_cnt%2==0)Gyro_PID();//4ms一次角速度环
    update_zebra_delayed_stop();//目标斑马线识别后继续运行1秒，再在中断中可靠停车
    if(car_running==1&&temp_run_flag==0)
    {
        speed_left_error_integral=0;
        speed_right_error_integral=0;
        Gyro_error_integral=0;
    }
    if(car_running == 0)
    {
        menu_isr_handler();   // 静止 → 菜单处理
        Motor_pwm_set(0,0);
        speed_left_error_integral=0;
        speed_right_error_integral=0;
        Gyro_error_integral=0;
        PWM_FAN_SET(0);
        encoder_left.clear_count();
        encoder_right.clear_count();
        run_time_cnt = 0;//停车将运行时间计数器清零
        temp_run_flag = car_running;
    }
    else    //小车行进处理
    {
        run_time_cnt++;//运行时间增加,前500ms方便人撤离
        if(run_time_cnt>250&&run_time_cnt<=500)//500ms-1000ms
        {
            PWM_FAN_SET(500+(10*FAN_SET-500)*0.5);
        }
        else if(run_time_cnt>500&&run_time_cnt<=1000)//1000ms-2000ms
        {
            PWM_FAN_SET(500+(10*FAN_SET-500)*0.75);
        }
        else if(run_time_cnt>1000)//2000ms后车辆启动
        {
            // if(evade_slow_lock==1)
            //     PWM_FAN_SET(10*FAN_SET_EVA);//检测到红块即切负压(与减速同步),不等推理完成
            // else
            PWM_FAN_SET(10*FAN_SET);
            //--------
            // speed_left_target=60;
            // speed_right_target=0;
            //--------
            if (card_stop_request == 1 || card_reverse_phase == CARD_REVERSE_BRAKING)
            {
                // 保持PIT、图像和NCNN线程运行，只强制两轮零速；不能使用car_running=0。
                speed_left_target = 0;
                speed_right_target = 0;
                speed_close_PID(0,0);
            }
            else
            {
                speed_close_PID(speed_left_target,speed_right_target);//2ms一次速度环
            }
            // speed_close_PID(0,0);//速度环目标速度为0，停车
            // speed_close_PID(base_speed,0);
        }
        // 运行中 → 只监听停止按键
        if(isr_cnt%60==0)//节省算力，120ms检测一次
        {
            if( key4_short_press_detect() == 0 )
            {
                car_running = 0;
                printf("停车原因: 按键停车(key4短按)\n");
            }
        }
    }
    if(isr_cnt>=600000) isr_cnt=0;//防止溢出/
    isr1_active.store(false, std::memory_order_seq_cst);
}

void isr2_handler()
{
   if(car_running == 0)
    {
        menu_isr_handler();   // 静止 → 菜单处理
    }
}

void sigint_handler(int signum)
{
    printf("停车原因: 收到Ctrl+C(SIGINT),程序退出\n");
    exit(0);
}
void cleanup()
{
    Motor_pwm_set(0, 0);   // 两个驱动电机 PWM 归零(停转)
    PWM_FAN_SET(0);        // 负压风扇(ESC)归零
    // 需要先停止定时器线程，后面才能稳定关闭电机，电调，舵机等
    pit_timer.stop();
    // 定时器已停 → ISR 不再刷新 PWM，此处直接把电机/负压清零，
    // 保证程序退出/崩溃/Ctrl+C 后车不再继续冲(防飞车)。
    car_running = 0;
    printf("停车原因: 程序退出/崩溃, cleanup已断电机+负压(若上面没打印具体原因,则是异常/崩溃)\n");
}
void pit_init()
{
    // 注册清理函数
    atexit(cleanup);
    //注册SIGINT信号的处理函数
    signal(SIGINT, sigint_handler);
    //创建一个定时器5ms周期，回调函数为isr1_handler
    pit_timer.init_ms(2, isr1_handler);
     //创建一个定时器200ms周期，回调函数为isr2_handler
    // pit_timer.init_ms(200, isr2_handler);
    // 系统初始化函数，进行必要的系统资源配置和初始化操作
    
}
