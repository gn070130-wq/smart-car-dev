#include "init.hpp"
zf_device_ips200 ips200;        // 定义2寸IPS SPI屏幕的设备对象，用于屏幕的所有操作
zf_device_uvc uvc_dev;          // 定义UVC免驱摄像头的设备对象，用于摄像头的所有操作
// 创建编码器对象，传入文件路径
zf_driver_encoder encoder_left(ENCODER_LEFT_PATH);
zf_driver_encoder encoder_right(ENCODER_RIGHT_PATH);
// IMU
zf_device_imu imu_dev;
zf_driver_gpio  key_1(KEY_1_PATH, O_RDWR);
zf_driver_gpio  key_2(KEY_2_PATH, O_RDWR);
zf_driver_gpio  key_3(KEY_3_PATH, O_RDWR);
zf_driver_gpio  key_4(KEY_4_PATH, O_RDWR);
zf_driver_gpio  key_5(KEY_5_PATH, O_RDWR);//中
zf_device_dl1x TOF;
uint8_t system_init(void)
{
    Parameter_Read();   //系统读取参数 
    //点击初始化
    drv8701e_pwm_left.get_dev_info(&drv8701e_pwm_left_info);
    drv8701e_pwm_left.get_dev_info(&drv8701e_pwm_right_info);
    pwm_fan.get_dev_info(&pwm_fan_info);
    // 初始化2寸IPS屏幕，传入帧缓冲设备路径，完成屏幕底层驱动、显示区域初始化
    ips200.init(FB_PATH);
    // IMU传感器初始化，自动检测挂载的IMU型号并完成底层配置
    imu_dev.init();
    //TOF测距初始化
    // TOF.init();
     // 初始化返回值小于0表示初始化失败，直接退出程序
    if(uvc_dev.init(UVC_PATH) < 0)
    {
        return -1;
    }
    return 0;           //初始化成功，返回0
}