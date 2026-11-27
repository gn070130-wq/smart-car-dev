
/*********************************************************************************************************************
* LS2K0300 Opensourec Library 即（LS2K0300 开源库）是一个基于官方 SDK 接口的第三方开源库
* Copyright (c) 2022 SEEKFREE 逐飞科技
********************************************************************************************************************/
//7.22完赛开始提速，鲁棒性检查
#include <atomic>

#include "zf_common_headfile.hpp"
#include "chassis.hpp"
#include "esc.hpp"
#include "Seeekfree_assistant.hpp"
#include "image_recognition.hpp"
#include "vision.hpp"

zf_driver_pit pit_timer;
zf_device_uvc uvc_dev;                
#if ENABLE_SEEKFREE_ASSISTANT
zf_driver_tcp_client tcp_client;
#endif
zf_driver_gpio beep(ZF_GPIO_BEEP, O_RDWR);

#define START_BEEP_DURATION_MS       (200U)
#define START_DELAY_MS               (1000U)
#define RED_AVOIDANCE_ENCODER_TARGET_COUNT (10000) // 绕行结束时左右编码器的平均计数
#define RED_RECOGNITION_MAX_ATTEMPTS (3U)       // 每轮最多连续识别3次
#define RED_RECOGNITION_DIRECT_CONFIDENCE (0.80f) // 单次达到80%可直接采用

 //==========================================全局变量定义===========================================//

uint8* rgay_image;                          // 灰度图像数据指针，指向摄像头采集到的灰度图像缓冲区首地址
#if ENABLE_SEEKFREE_ASSISTANT
uint8 image_copy[UVC_HEIGHT][UVC_WIDTH];    // 灰度图像拷贝缓冲区，保存缩放后的160x120处理帧
#endif
VisionResult g_vision_result;               // 视觉处理结果结构体，包含边界信息、误差等，供底盘控制使用
extern float Target_Speed_Left;
extern float Target_Speed_Right;
std::atomic_bool g_final_stop_requested(false);


void pit_callback()
{
    if(g_final_stop_requested.load())
    {
        Target_Speed_Left = 0.0f;
        Target_Speed_Right = 0.0f;
    }
    else
    {
        chassis_gyro_control_tick();  //底盘陀螺仪中环控制函数，根据目标角速度调整左右轮目标速度
    }
    chassis_speed_control_tick();     //底盘速度内环控制函数，根据目标速度调整左右电机PWM
}

void sigint_handler(int signum) 
{
    printf("收到Ctrl+C,程序即将退出\n");
    exit(0);
}

void cleanup()
{
    // 需要先停止定时器线程，后面才能稳定关闭电机，电调，舵机等
    pit_timer.stop();
    printf("程序退出，执行清理操作\n");
    beep.set_level(0);
    // 关闭电机，电调，舵机等
    esc_stop();
    chassis_stop();
}


int main(int, char **)
{
    atexit(cleanup);                            //注册清理函数
    signal(SIGINT, sigint_handler);             // 注册SIGINT信号的处理函数

    //============================================================ 函数初始化 ======================================================================//
    chassis_init();                                                                         //底盘初始化函数
    if(0 != uvc_dev.init(UVC_PATH))                                                         //摄像头初始化函数
    {
        printf("uvc device init failed\r\n");
        return -1;
    }
    if(!image_recognition_init())
    {
        printf("image recognition init failed\r\n");
        return -1;
    }

    printf("校准中...\r\n");     //陀螺仪偏移量获取
    yaw_offset_calibration(); //陀螺仪Z轴零偏校准，需在水平静止状态下调用
    printf("校准完成\r\n");

    //============================================================ 通信初始化 ======================================================================//
#if ENABLE_SEEKFREE_ASSISTANT
    seekfree_assistant_debug_init(&tcp_client, image_copy[0], UVC_WIDTH, UVC_HEIGHT);
#endif

    // 先启动电调，完成后蜂鸣一声，再等待1秒发车。
    esc_init();

    beep.set_level(1);
    system_delay_ms(START_BEEP_DURATION_MS);
    beep.set_level(0);
    system_delay_ms(START_DELAY_MS);

   //=============================================================== 定时器初始化 ======================================================================//
    pit_timer.init_ms(5, pit_callback);        // 启动5ms周期底盘控制定时器，车辆开始行驶

    //==================================================== 主循环：持续采集灰度图像 + TCP发送 ========================================================//


    
    
    uint8_t recognition_attempt_count = 0;
    uint8_t completed_recognition_count = 0;
    int previous_recognition_class_id = -1;

    while (true) 
    {
#if ENABLE_SEEKFREE_ASSISTANT
        seekfree_assistant_debug_update_parameters();
        chassis_send_debug_waveform(g_vision_result);
#endif
        // 阻塞式等待摄像头完成新一帧图像的采集刷新，返回<0表示采集失败/异常
        if(uvc_dev.wait_image_refresh() < 0)
        {
            exit(0);   // 摄像头采集异常，直接退出程序，防止卡死
        }

        // 获取摄像头采集到的灰度图像数据缓冲区首地址
        rgay_image = uvc_dev.get_gray_image_ptr();
        cv::Mat red_detection_image = uvc_dev.get_frame_resized();
        cv::Mat original_image = uvc_dev.get_frame_mjpg();
        Process_Vision_Frame(rgay_image, red_detection_image, g_vision_result);  // 160x120 灰度图巡线，160x120 彩色图检测红块

        if(RED_AVOIDANCE_STATE_WAIT_DIRECTION == g_vision_result.red_avoidance_state &&
           g_vision_result.red_y + g_vision_result.red_h > RED_RECOGNITION_BOTTOM_ROW)
        {
            ImageRecognitionResult recognition_result;
            if(image_recognition_infer(original_image, g_vision_result, recognition_result))
            {
                if(completed_recognition_count < 5U)
                {
                    image_recognition_print(recognition_result);
                }
                ++recognition_attempt_count;
                if(recognition_result.confidence >= RED_RECOGNITION_DIRECT_CONFIDENCE ||
                   previous_recognition_class_id == recognition_result.class_id)
                {
                    if(completed_recognition_count >= 5U)
                    {
                        g_final_stop_requested.store(true);
                        image_recognition_print(recognition_result);
                        while(true)
                        {
                            system_delay_ms(1000U);
                        }
                    }
                    else
                    {
                        Set_Red_Avoidance_Direction(g_vision_result, recognition_result.direction);
                        Red_Avoidance_State_Machine(g_vision_result);
                    }
                }

                if(RED_AVOIDANCE_STATE_WAIT_DIRECTION != g_vision_result.red_avoidance_state ||
                   recognition_attempt_count >= RED_RECOGNITION_MAX_ATTEMPTS)
                {
                    recognition_attempt_count = 0;
                    previous_recognition_class_id = -1;
                }
                else
                {
                    previous_recognition_class_id = recognition_result.class_id;
                }
            }
            else
            {
                recognition_attempt_count = 0;
                previous_recognition_class_id = -1;
            }
        }
        else
        {
            recognition_attempt_count = 0;
            previous_recognition_class_id = -1;
        }

        if(chassis_red_avoidance_encoder_reached(RED_AVOIDANCE_ENCODER_TARGET_COUNT))
        {
            ++completed_recognition_count;
            Start_Red_Avoidance_Return(g_vision_result);
        }
        chassis_vision_control_tick(g_vision_result);

#if ENABLE_SEEKFREE_ASSISTANT
        if(seekfree_assistant_debug_should_send_camera())
        {
            seekfree_assistant_debug_send_camera_frame(rgay_image, image_copy, g_vision_result);
        }
#endif
    }

    return 0;  // 死循环不会执行到此行，程序正常退出返回值

}
