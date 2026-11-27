#ifndef __INIT_hpp__
#define __INIT_hpp__
#include  "zf_common_headfile.hpp"
extern zf_device_ips200 ips200;      
extern zf_device_uvc uvc_dev; 
extern zf_driver_encoder encoder_left;
extern zf_driver_encoder encoder_right;
extern zf_device_imu imu_dev;       
extern zf_device_dl1x TOF;
extern zf_driver_gpio  key_1;//中
extern zf_driver_gpio  key_2;//上
extern zf_driver_gpio  key_3;//下
extern zf_driver_gpio  key_4;//左
extern zf_driver_gpio  key_5;//右
uint8_t system_init(void); // 系统初始化函数，进行必要的系统资源配置和初始化操作
#endif
