#include "zf_common_headfile.hpp"
#include "pid.h"
#include "image.h"
#include "menu.h"
#include "cross.h" 
#include "ring.h" 
#include "preprocess.h" 
#include "imu.h"
#include "fuzzy.h"
#include <execinfo.h>

#include "ncnn_app.hpp"
#include <opencv2/opencv.hpp>

#define SERVER_IP   "10.81.8.189"
#define PORT        8086

#include <chrono>


zf_driver_tcp_client tcp_client_dev;
zf_device_uvc uvc_dev;
zf_device_imu imu_dev;

uint32  read_len = 0;
uint8   recv_buff[1024];
uint8   temp_str[] = "seekfree this is tcp_client demo.\r\n";
uint8   read_str[] = "read data:\r\n";


//蜂鸣器初始化
#define BEEP_PATH        ZF_GPIO_BEEP
zf_driver_gpio  beep(BEEP_PATH, O_RDWR);

//PWM初始化
#define PWM_1_PATH        ZF_PWM_MOTOR_1
#define DIR_1_PATH        ZF_GPIO_MOTOR_1
#define PWM_2_PATH        ZF_PWM_MOTOR_2
#define DIR_2_PATH        ZF_GPIO_MOTOR_2
struct pwm_info drv8701e_pwm_1_info;
struct pwm_info drv8701e_pwm_2_info;
zf_driver_gpio  drv8701e_dir_1(DIR_1_PATH, O_RDWR);
zf_driver_gpio  drv8701e_dir_2(DIR_2_PATH, O_RDWR);
zf_driver_pwm   drv8701e_pwm_1(PWM_1_PATH);
zf_driver_pwm   drv8701e_pwm_2(PWM_2_PATH);

//编码器初始化
int16 encoder_left, encoder_right;                              // 左右轮转速（10ms 累计脉冲数）
zf_driver_encoder encoder_quad_1(ZF_ENCODER_QUAD_1);           // 左编码器
zf_driver_encoder encoder_quad_2(ZF_ENCODER_QUAD_2);           // 右编码器


zf_driver_pit pit_timer;
zf_device_ips200 ips200;

uint32 tcp_send_wrap(const uint8 *buf, uint32 len)
{
    return tcp_client_dev.send_data(buf, len);
}

uint32 tcp_read_wrap( uint8 *buf, uint32 len)
{
    return tcp_client_dev.read_data(buf, len);
}

void signal_handler(int signum) {
    if (signum == SIGSEGV) 
    {
        printf("\n[Error] Segmentation fault detected!\n");
        
        void *array[10];
        size_t size = backtrace(array, 10);

        printf("--- Stack Trace (Call History) ---\n");
        backtrace_symbols_fd(array, size, STDOUT_FILENO);
        printf("----------------------------------\n");
        
    }
    else if (signum == SIGINT) 
    {
        printf("\n[Info] Interrupt signal (Ctrl+C) received.\n");
    }

    // exit 会自动触发 atexit 注册的清理函数
    exit(signum == SIGSEGV ? EXIT_FAILURE : EXIT_SUCCESS);
}


void cleanup()
{
    // 需要先停止定时器线程，后面才能稳定关闭电机，电调，舵机等
    pit_timer.stop();
    printf("程序异常退出，执行清理操作\n");
    // 关闭电机
    drv8701e_pwm_1.set_duty(0);   
    drv8701e_pwm_2.set_duty(0);    
}



// 灰度图像数据拷贝缓冲区，二维数组存储，大小与摄像头采集分辨率一致，用于中转图像数据
uint8 image_copy[UVC_HEIGHT][UVC_WIDTH];

uint8_t start_flag=0;
uint8_t send_data_flag = 0;

int baseSpeed=150;
float Imgerrtmp=0;

float filtered_left  = 0.0f;
float filtered_right = 0.0f;

float gyro_z_actual=0;
float target_yaw_rate=0;

int count=0,t1=0;
extern int l_duty,r_duty;

float ring_exit_distance = 0.0f;//编码器积分

void pit_callback(void)
{
    static uint8_t time_count = 0; // 核心：用于分频的计数器
    time_count++;

    encoder_left  = encoder_quad_1.get_count();
    encoder_right = -encoder_quad_2.get_count();
    encoder_quad_1.clear_count();
    encoder_quad_2.clear_count();

    filtered_left  = encoder_left;
    filtered_right = encoder_right;
    
    //10ms方向环
    if (time_count % 2 == 0) 
    {
        target_yaw_rate=Positional_PID_Calc(&Position,Imgerrtmp,70);
    }
    
    //5ms角速度环
    int16_t raw = imu_dev.get_gyro_z();
    float gyro_z_actual = ((float)raw - gyro_int.raw_offset) / GYRO_SENSITIVITY;
    float steer_output = Gyro_PID_Calc(&Gyro_PID,target_yaw_rate,gyro_z_actual);
    

    //5ms速度环
    float leftTarget  = baseSpeed + steer_output;
    float rightTarget = baseSpeed - steer_output;

    // float diff = steer_output;          // 转向差量（有正负）
    // float outer_ratio = 1.0f;           // 外侧轮系数（加速侧）
    // float inner_ratio = 1.5f;           // 内侧轮系数（减速侧）-> 这就是“内减多一点”

    // float leftTarget, rightTarget;

    // if (diff >= 0)  // diff > 0 表示需要右转（右轮为内侧）
    // {
    //     leftTarget  = baseSpeed + diff * outer_ratio;   // 左外轮：正常加速
    //     rightTarget = baseSpeed - diff * inner_ratio;   // 右内轮：大幅减速（内减多）
    // } 
    // else            // diff < 0 表示需要左转（左轮为内侧）
    // {
    //     diff = -diff;                                   // 转成正数方便计算
    //     leftTarget  = baseSpeed - diff * inner_ratio;   // 左内轮：大幅减速（内减多）
    //     rightTarget = baseSpeed + diff * outer_ratio;   // 右外轮：正常加速
    // }

    // // 【重要】防止目标速度为负导致电机卡死或反转失控
    // if (leftTarget < -15)  leftTarget = -15;   // 若你的电机允许反转刹车，可保留负值，否则设为0
    // if (rightTarget < -15) rightTarget = -15;

    if(start_flag)
    {
        Motor_Control(leftTarget,rightTarget);
    }
    else
    {
        drv8701e_pwm_1.set_duty(0);
        drv8701e_pwm_2.set_duty(0);
    }

    //遇到圆环开启积分
    if(roadstate==Ring_Road)
    {
        gyro_integrator_update();        
    }

    //右出圆环开启积分
    if(roadstate == Ring_Road && ringstate == Right_outRing)
    {
        ring_exit_distance += (filtered_left + filtered_right) * 0.5f; // 取平均
    }


    // seekfree_assistant_oscilloscope_data.head = SEEKFREE_ASSISTANT_SEND_HEAD;
    // seekfree_assistant_oscilloscope_data.channel_num = 8;      
    // seekfree_assistant_oscilloscope_data.data[0] = (float)(filtered_left);
    // seekfree_assistant_oscilloscope_data.data[1] = (float)(Left_motor.target_speed);
    // seekfree_assistant_oscilloscope_data.data[2] = (float)(filtered_right);
    // seekfree_assistant_oscilloscope_data.data[3] = (float)(Right_motor.target_speed);
    // seekfree_assistant_oscilloscope_data.data[4] = (float)(l_duty);
    // seekfree_assistant_oscilloscope_data.data[5] = (float)(r_duty);
    // seekfree_assistant_oscilloscope_data.data[6] = (float)(gyro_z_actual);
    // seekfree_assistant_oscilloscope_data.data[7] = (float)(target_yaw_rate);   
    // seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);

    if(roadstate==Ring_Road&&(ringstate==Right_intoWeak||ringstate==Left_intoWeak))
    {
        count++;
    }
    else if(roadstate==Ring_Road&&(ringstate==Right_inRing||ringstate==Left_inRing))
    {
        count=0;
    }
    if(roadstate==Ring_Road&&(ringstate==Right_outRing||ringstate==Left_outRing))
    {
        t1++;
    }

}



extern int c;
int flag=0;//斑马线


int main(int, char**) 
{

    //---------------------------------上位机---------------------------------------
    // if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
    // {
    //     printf("tcp_client ok\r\n");    // TCP连接成功，串口打印成功日志
    // }
    // else
    // {
    //     printf("tcp_client error\r\n"); // TCP连接失败，串口打印错误日志
    //     return -1;                      // 连接失败直接退出程序，不执行后续流程
    // }
    
    // seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
    // seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_SCC8660, color_image[0], WIDTH, HEIGHT);
    
    if(uvc_dev.init(UVC_PATH) < 0)
    {
        return -1;  // 摄像头初始化失败，直接退出程序
    }

    //---------------------------------初始化---------------------------------------
    drv8701e_pwm_1.get_dev_info(&drv8701e_pwm_1_info);
    drv8701e_pwm_2.get_dev_info(&drv8701e_pwm_2_info);

    
    atexit(cleanup);// 注册清理函数
    signal(SIGSEGV, signal_handler);  // 段错误
    signal(SIGINT, signal_handler); // 注册SIGINT信号的处理函数

    ips200.init(FB_PATH);
    imu_dev.init();
    pit_timer.init_ms(5, pit_callback);// 创建一个定时器5ms周期，回调函数为pit_callback


    //PID初始化
    Position_Init(&Position,7.5,0,0,4000);
    Gyro_PID_Init(&Gyro_PID,1.3,0,0,120);

    Incremental_Motor_Init(&Left_motor,6.5,0.75,0,5000);
    Incremental_Motor_Init(&Right_motor,6.5,0.75,0,5000);

    uint8_t KeyNum=0;

    system_delay_ms(500);         // 等待 500ms 让传感器完全稳定
    gyro_integrator_init();
    gyro_integrator_calibrate(100);//零偏

    while(1)
    {

        if(uvc_dev.wait_image_refresh() < 0)
        {
            exit(0);   // 摄像头采集异常，直接退出程序，防止卡死
        }

        cv::Mat frame = uvc_dev.get_frame_mjpg();//获取彩色图像
        if(frame.empty()) continue;

        cv::Mat gray_frame;//将彩色图像转换为灰白图像
        cv::cvtColor(frame, gray_frame, cv::COLOR_BGR2GRAY);

        image_process(gray_frame.data);
        
        //---------------------------------特殊元素---------------------------------------
        //十字判断，判断上一帧是否为常规路段或十字路段
        if((roadstate == Normal_Road || roadstate == Cross_Road))
        {
            do
            {
                //判断十字状态
                cross_state = isCrossRoad();
                if(cross_state == 0) {
                    roadstate = Normal_Road;
                    cross_state = no_crossRoad;
                    //非十字退出不进行后续操作
                    break;
                } else {
                    //进入十字状态并进行处理
                    roadstate = Cross_Road;
                }
                
                //十字处理
                crossRoad_processing();

            } while (0);

        }
        
        //圆环判断，判断上一帧是否为常规路段或圆环路段
        if((roadstate == Normal_Road || roadstate == Ring_Road)&& ringcount <RingCounts)//圆环数量大于0
        {
            //右圆环状态检测
            RightRing_State_checking();

            //左圆环状态检测
            LeftRing_State_checking();
            
            if(roadstate == Ring_Road)
            {
                ring_processing();
            }
        
        }
        
        //常规路段
        if(roadstate == Normal_Road)
        {
            if(checking_Normal_straightaway()) {
                normalstate = Normal_longway;
            } else {
                normalstate = Normal_curve;
            }
            normalRoad_decusion();
        }

        int trackAim=0;

        if(roadstate==Normal_Road&&normalstate==Normal_longway)
        {
            trackAim = 50;
        }

        else
        {
            trackAim = 20;
        }

        switch (ringstate)
        {
            case Right_outRing:
                trackAim = 50;
                break;
            
            case Left_outRing:
                trackAim = 50;
                break;
            case Right_intoRing:
                trackAim = 30;
                break;
            
            case Left_intoRing:
                trackAim = 30;
                break;
            default:
                trackAim = 20;
                break;
        }

        //提取误差
        Imgerrtmp = get_roadError(trackAim);

        // //---------------------------------上位机图传---------------------------------------
        // generate_color_image_for_upload();
        // int b = (left_Line.pointsNum > right_Line.pointsNum) ? right_Line.pointsNum : left_Line.pointsNum;
        // for (int i = 0; i < b; ++i) 
        // {     
        //     int x1 =left_Line.edgelinepoint[i][1];
        //     int y1 =left_Line.edgelinepoint[i][0];

        //     int x2 =right_Line.edgelinepoint[i][1];
        //     int y2 =right_Line.edgelinepoint[i][0];  

        //     int x3 =midline.edgelinepoint[i][1];
        //     int y3 =midline.edgelinepoint[i][0];
            
        //     drawPixelOnColor(color_image, x1, y1, COLOR_RED);   
        //     drawPixelOnColor(color_image, x2, y2, COLOR_GREEN);   
        //     drawPixelOnColor(color_image, x3, y3, COLOR_BLUE);   
        // }
        // for(int y = yend - 4; y > maxWhite.maxy; y--)
        // {
        //     drawPixelOnColor(color_image, maxWhite.starlineX, y, COLOR_BLACK);
        //     drawPixelOnColor(color_image, maxWhite.endlineX, y, COLOR_BLACK);
        // }
        // if(left_up_mutation[2])
        // drawFilledCircleOnColor(color_image, left_up_mutation[1], left_up_mutation[0],1,COLOR_BLUE);   
        // if(left_points_turnUp[2])
        // drawFilledCircleOnColor(color_image, left_points_turnUp[1], left_points_turnUp[0],1,COLOR_GREEN);   
        // if(left_points_turnDown[2])
        // drawFilledCircleOnColor(color_image, left_points_turnDown[1], left_points_turnDown[0],1,COLOR_GREEN);   
        // if(right_up_mutation[2])
        // drawFilledCircleOnColor(color_image, right_up_mutation[1], right_up_mutation[0],1,COLOR_BLUE);   
        // if(right_points_turnUp[2])
        // drawFilledCircleOnColor(color_image, right_points_turnUp[1], right_points_turnUp[0],1,COLOR_RED);   
        // if(right_points_turnDown[2])
        // drawFilledCircleOnColor(color_image, right_points_turnDown[1], right_points_turnDown[0],1,COLOR_RED);   

        // draw_digit_on_color(color_image, 1, 1,  roadstate,         0xF800, 0x0000); // 红色
        // draw_digit_on_color(color_image, 1, 11, normalstate,            0x07E0, 0x0000); // 绿色
        // draw_digit_on_color(color_image, 1, 21, cross_state,   0x001F, 0x0000); // 蓝色
        // draw_digit_on_color(color_image, 1, 31, ringstate,       0xFFE0, 0x0000); // 黄色
        // seekfree_assistant_camera_send();
        
        //---------------------------------按键---------------------------------------
        KeyNum=Key_GetNum();
        switch(KeyNum)
        {
            case 1:
                start_flag = !start_flag;
                break;
        }

        // ips200.show_float(60,0,Left_motor.ki,1,2);
        // ips200.show_float(0,0,Left_motor.kp,1,2);
        // ips200.show_float(60,20,Right_motor.ki,1,2);
        // ips200.show_float(0,20,Right_motor.kp,1,2);

        // ips200.show_float(60,40,Gyro_PID.kd,1,2);
        // ips200.show_float(0,40,Gyro_PID.kp,1,2);
        // ips200.show_float(60,60,Position.kd,1,2);
        // ips200.show_float(0,60,Position.kp,1,2);

        // //---------------------------------屏幕---------------------------------------

        // ips200.show_int(0,0,roadstate,1);
        // ips200.show_int(0,20,normalstate,1); 
        // ips200.show_int(0,40,cross_state,1);
        // ips200.show_int(0,60,ringstate,1); 

        // ips200.show_string(0, 80, "tu");
        // ips200.show_int(100,80,left_up_mutation[2],1);
        // ips200.show_int(120,80,right_up_mutation[2],1); 
        
        // ips200.show_string(0, 100, "shang");
        // ips200.show_int(100,100,left_points_turnUp[2],1);
        // ips200.show_int(120,100,right_points_turnUp[2],1); 

        // ips200.show_string(0, 120, "xia");
        // ips200.show_int(100,120,left_points_turnDown[2],1);
        // ips200.show_int(120,120,right_points_turnDown[2],1); 
        
        // ips200.show_float(0,140, gyro_int.integration, 3, 2);  // 改为 filtered_integration
        //ips200.show_int(0,0,flag,1);
    
    }
}
