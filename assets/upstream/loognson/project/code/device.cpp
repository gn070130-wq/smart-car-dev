#include "device.h"

// device initial
zf_device_ips200 ips200;                    // 定义IPS200屏幕的设备对象
zf_device_uvc uvc_dev;                      // 定义UVC免驱摄像头的设备对象
zf_driver_tcp_client tcp_client_dev;        // 定义TCP客户端设备对象
zf_device_imu imu_dev;                      // 定义陀螺仪的设备对象
zf_driver_pit pit_timer;                    // 定义PIT定时器的设备对象
zf_device_dl1x dl1x_dev;                    // DL1X设备对象
enum dl1x_device_type_enum dl1x_dev_type;   // DL1X设备类型
pwm_info drv8701e_pwm_1_info;
pwm_info drv8701e_pwm_2_info;
struct pwm_info esc_info;
zf_driver_pwm esc_pwm(ESC_PATH);
// TCP
uint16_t rgb_image_tcp[UVC_HEIGHT][UVC_WIDTH];
uint8_t gray_image_tcp[ROW][COL];
uint8_t image_buffer_tcp[ROW][COL];
// initial
volatile int16_t dl1x_distance = 0;
bool system_self_lock = false;
bool green_detect = false;
zf_driver_gpio  beep(BEEP_PATH, O_RDWR);
#if MODEL_SELECTION == 1
// TFLite模型处理类对象
TFLiteCameraClassifier TFLiteclassifier;
#elif MODEL_SELECTION == 2
// NCNN模型处理类对象
NCNNCameraClassifier NCNNclassifier;
#endif
// 模型ROI显示相关变量
static constexpr uint8_t MODEL_ROI_HOLD_FRAMES = 10;
static cv::Rect model_roi_display_rect;
static bool model_roi_display_valid = false;
static uint8_t model_roi_display_countdown = 0;
static uint8_t prev_model_type_for_display = 0;
static uint8_t prev_last_model_type_for_display = 0;

/****************************** model display ******************************/
/**
 * @brief 模型ROI显示状态更新
 * @param void
 * @return void
 * @note 更新模型ROI显示状态，根据模型输出结果和上一帧的模型输出结果判断是否需要更新显示状态
 * @author mx
 */
static inline uint16_t swap_u16(uint16_t v)
{
    return (uint16_t)((v << 8) | (v >> 8));
}

/**
 * @brief 模型ROI显示状态更新
 * @param void
 * @return void
 * @note 更新模型ROI显示状态，根据模型输出结果和上一帧的模型输出结果判断是否需要更新显示状态
 * @author mx
 */
static void update_model_roi_display_state(void)
{
    // 检查是否有新的模型输出结果
    bool new_model_output = ((model_type != 0) && (prev_model_type_for_display == 0))
        || ((last_model_type != 0) && (prev_last_model_type_for_display == 0));
    // 如果有新的模型输出结果，且模型输出结果包含ROI矩形
    // 则更新模型ROI显示状态
    if(new_model_output && Active_classifier.hasRoiRect())
    {
        model_roi_display_rect = Active_classifier.getRoiRect();
        model_roi_display_valid = true;
        model_roi_display_countdown = MODEL_ROI_HOLD_FRAMES;
    }
    else if(model_roi_display_countdown > 0)
    {
        model_roi_display_countdown--;
        if(model_roi_display_countdown == 0)
        {
            model_roi_display_valid = false;
        }
    }
    // 更新上一帧的模型输出结果
    prev_model_type_for_display = model_type;
    prev_last_model_type_for_display = last_model_type;
}

/**
 * @brief 绘制矩形
 * @param buf 像素缓冲区指针
 * @param width 图像宽度
 * @param height 图像高度
 * @param x 矩形左上角x坐标
 * @param y 矩形左上角y坐标
 * @param w 矩形宽度
 * @param h 矩形高度
 * @param color 矩形颜色
 * @return void
 * @note 绘制矩形，根据buf、width、height、x、y、w、h参数绘制矩形
 * @author mx
 */
void draw_rect_rgb565(uint16_t* buf, int16_t width, int16_t height, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    if(buf == nullptr || width <= 0 || height <= 0 || w <= 0 || h <= 0)
    {
        return;
    }
    // 计算矩形的四个顶点坐标
    int16_t x0 = x;
    int16_t y0 = y;
    int16_t x1 = x + w - 1;
    int16_t y1 = y + h - 1;
    // 矮化坐标范围，确保矩形在图像范围内
    if(x0 < 0) x0 = 0;
    if(y0 < 0) y0 = 0;
    if(x1 >= width) x1 = width - 1;
    if(y1 >= height) y1 = height - 1;
    if(x0 > x1 || y0 > y1)
    {
        return;
    }
    // 绘制矩形
    for(int16_t xx = x0; xx <= x1; xx++)
    {
        buf[y0 * width + xx] = color;
        buf[y1 * width + xx] = color;
    }
    for(int16_t yy = y0; yy <= y1; yy++)
    {
        buf[yy * width + x0] = color;
        buf[yy * width + x1] = color;
    }
}
/****************************** model display ******************************/

/****************************** TCP ******************************/
/**
 * @brief TCP发送数据
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @return uint32_t 发送字节数
 * @note 发送数据时，需要先初始化TCP客户端设备对象
 * @author YuYu
 */
uint32_t tcp_send_wrap(const uint8_t *buf, uint32_t len)
{
    return tcp_client_dev.send_data(buf, len);
}

/**
 * @brief TCP接收数据
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @return uint32_t 接收字节数
 * @note 接收数据时，需要先初始化TCP客户端设备对象
 * @author YuYu
 */
uint32_t tcp_read_wrap( uint8_t *buf, uint32_t len)
{
    return tcp_client_dev.read_data(buf, len);
}

/**
 * @brief TCP初始化
 * @param void
 * @return int8_t 初始化结果
 * @note 初始化TCP客户端设备对象，连接到指定的IP地址和端口号
 * @author YuYu
 */
int8_t TCP_initial(void)
{
    if(1 == USE_TCP_SERVER)
    {
        if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
        {
            printf("tcp_client ok\r\n");    // TCP连接成功，串口打印成功日志
        }
        else
        {
            return -1;                      // 连接失败直接退出程序，不执行后续流程
        }
        seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
        seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_SCC8660, rgb_image_tcp[0], UVC_WIDTH, UVC_HEIGHT);
    }
    else if(2 == USE_TCP_SERVER)
    {
        if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
        {
            printf("tcp_client ok\r\n");    // TCP连接成功，串口打印成功日志
        }
        else
        {
            return -1;                      // 连接失败直接退出程序，不执行后续流程
        }
        // 初始化逐飞助手通信接口
        seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
        // 配置摄像头信息
        seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, gray_image_tcp[0], COL, ROW);
    }
    else if(3 == USE_TCP_SERVER)
    {
        if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
        {
            printf("tcp_client ok\r\n");    // TCP连接成功，串口打印成功日志
        }
        else
        {
            return -1;                      // 连接失败直接退出程序，不执行后续流程
        }
        // 初始化逐飞助手通信接口
        seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
        // 配置摄像头信息
        seekfree_assistant_camera_information_config(SEEKFREE_ASSISTANT_MT9V03X, image_buffer_tcp[0], COL, ROW);
    }
    else if(4 == USE_TCP_SERVER)
    {
        if(tcp_client_dev.init(SERVER_IP, PORT) == 0)
        {
            printf("tcp_client ok\r\n");    // TCP连接成功，串口打印成功日志
        }
        else
        {
            return -1;                      // 连接失败直接退出程序，不执行后续流程
        }
        seekfree_assistant_interface_init(tcp_send_wrap, tcp_read_wrap);
    }
    return 0;
}

/**
 * @brief TCP发送数据
 * @param buf 数据缓冲区指针
 * @param len 数据长度
 * @return void
 * @note 发送数据时，需要先初始化TCP客户端设备对象
 * @author YuYu
 */
void TCP_send_data(void)
{
    if(1 == USE_TCP_SERVER)
    {
        update_model_roi_display_state();
        // 图像数据格式转换：高低位字节交换(摄像头采集的RGB565数据高低位顺序与上位机解析要求不一致，必须交换后才能正常显示彩色图像)
        for(uint32_t i = 0; i < UVC_WIDTH * UVC_HEIGHT; i++)
        {
            ((uint16_t *)rgb_image_tcp[0])[i] = (((uint16_t *)rgb_image)[i] & 0xFF) <<8 | (((uint16_t *)rgb_image)[i] & 0xFF00) >>8;
        }
        if(SHOW_MODEL_ROI && model_roi_display_valid && model_roi_display_countdown > 0)
        {
            const cv::Rect r = model_roi_display_rect;
            draw_rect_rgb565((uint16_t*)rgb_image_tcp[0], UVC_WIDTH, UVC_HEIGHT, r.x, r.y, r.width, r.height, swap_u16(0xF800));
        }
        seekfree_assistant_camera_send();
    }
    else if(2 == USE_TCP_SERVER)
    {
        memcpy(gray_image_tcp[0], gray_image, COL * ROW);
        seekfree_assistant_camera_send();
    }
    else if(3 == USE_TCP_SERVER)
    {
        display_image();
        memcpy(image_buffer_tcp[0], image_buffer, COL * ROW);
        seekfree_assistant_camera_send();
    }
    else if(4 == USE_TCP_SERVER)
    {
        wifi_send_data();
    }
}

/**
 * @brief WIFI发送数据
 * @param void
 * @return void
 * @note 发送数据时，需要先初始化WIFI客户端设备对象
 * @author YuYu
 */
void wifi_send_data(void)
{
    // [诊断模式] 完整PID数据采集
    seekfree_assistant_oscilloscope_data.data[0] = gyro_dir_inner.set;           // ① 设定值(40)
    seekfree_assistant_oscilloscope_data.data[1] = gyro_dir_inner.actual;        // ② 实际值(滤波后)
    seekfree_assistant_oscilloscope_data.data[2] = gyro.angular_velocity.yaw;     // ③ 原始陀螺仪数据(未滤波)
    seekfree_assistant_oscilloscope_data.data[3] = gyro_dir_inner.err;            // ④ 误差(set-actual)
    seekfree_assistant_oscilloscope_data.data[4] = gyro_dir_inner.out;            // ⑤ PID原始输出(未限幅)
    seekfree_assistant_oscilloscope_data.data[5] = gyro_dir_inner.actual_out;     // ⑥ PID最终输出(已限幅)
    seekfree_assistant_oscilloscope_data.data[6] = differ_dir_outer.actual_out;   // ⑦ 外环输出(应该≈0)
    // 设置本次需要发送几个通道的数据, 最大支持8通道
    seekfree_assistant_oscilloscope_data.channel_num = 7;
    seekfree_assistant_oscilloscope_send(&seekfree_assistant_oscilloscope_data);     
    // system_delay_ms(20);
    seekfree_assistant_data_analysis();
}
/****************************** TCP ******************************/

/****************************** device initial ******************************/
/**
 * @brief 自检
 * @param void
 * @return int8_t 自检结果
 * @note 自检设备是否正常工作，包括TCP客户端设备对象、UVC免驱摄像头、陀螺仪、DL1X传感器、PID参数、菜单、定时器等
 * @author YuYu
 */
int8_t device_self_detect(void)
{
    if(TCP_initial() < 0)
    {
        printf("tcp_client error\r\n"); // TCP连接失败，串口打印错误日志
        return -1;
    }
    // 蜂鸣器初始化
    beep_enable(0);
    // 电机初始化
    motor_init();
    // 初始化2寸IPS屏幕，传入帧缓冲设备路径，完成屏幕底层驱动、显示区域初始化
    ips200.init(FB_PATH);
    // 初始化UVC免驱摄像头，传入USB摄像头设备路径
    if(uvc_dev.init(UVC_PATH) < 0)
    {
        printf("UVC camera initial error.\r\n");
        return -1;
    }
    #if MODEL_SELECTION == 1
    if (!TFLiteclassifier.init(&uvc_dev))
    {
        return -1;
    }
    #elif MODEL_SELECTION == 2
    NCNNclassifier.SetInputSize(NCNN_INPUT_WIDTH, NCNN_INPUT_HEIGHT);
    #if NCNN_OUTPUT_CLASS_NUM == 3
    NCNNclassifier.SetLabels({"Supplies", "Vehicle", "Weapon"});
    #elif NCNN_OUTPUT_CLASS_NUM == 6
    NCNNclassifier.SetLabels({"Firearms", "Explosive", "First Aid Kit", "Telescope", "Armoured", "Ambulance"});
    #endif
    if (!NCNNclassifier.init())
    {
        return -1;
    }
    #endif
    // 初始化陀螺仪，自动检测挂载的IMU型号并完成底层配置
    imu_dev.init();
    gyroscope_init();
    // // DL1X传感器初始化
    // dl1x_dev_type = dl1x_dev.init();
    // if(NO_FIND_DEVICE == dl1x_dev_type)
    // {
    //     printf("DL1X initial error.\r\n");
    //     return -1;
    // }
    // PID参数初始化
    pid_inc_init();
    dir_inner_init();
    differ_outer_init();
    // 菜单初始化
    UI_menu_init();
    #if 1 == USE_WECHAT_APP
    WeChat_UDP_init();
    #endif
    #if 1 == USE_BLDC
    // 电调初始化
    esc_init();
    #endif
    // 定时器初始化
    pit_timer.init_ms(5, pit_callback);
    return 0;
}

/**
 * @brief 蜂鸣器使能
 * @param enable 蜂鸣器使能状态
 * @return void
 * @note 使能蜂鸣器，根据enable参数判断是否使能
 * @author YuYu
 */
void beep_enable(bool enable)
{
    switch(enable)
    {
        case true:
            beep.set_level(1);
            break;
        case false:
            beep.set_level(0);
            break;
    }
}

/**
 * @brief 电调初始化
 * @param void
 * @return void
 * @note 初始化电调，获取电调PWM引脚信息
 * @author YuYu
 */
void esc_init(void)
{
    esc_pwm.get_dev_info(&esc_info);
    printf("esc pwm freq = %d Hz\r\n", esc_info.freq);
    printf("esc pwm duty_max = %d\r\n", esc_info.duty_max);
    printf("[SUCCESS]ESC Init ok.\r\n");
}

/**
 * @brief 电调设置占空比
 * @param duty 占空比
 * @return void
 * @note 设置电调占空比，根据duty参数设置占空比
 * @author YuYu
 */
void esc_set_duty(uint16_t duty)
{
    esc_pwm.set_duty(duty);
}
/****************************** device initial ******************************/
