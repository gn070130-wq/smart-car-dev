#ifndef _DEVICE_H_
#define _DEVICE_H_

#include "zf_common_headfile.hpp"

/****************************** TCP网络配置 ******************************/
#define  USE_TCP_SERVER      (1)              // 0：不开启TCP 1：TCP传输RGB彩色图像 2：TCP传输灰度图像 3：TCP传输赛道图像 4：示波器示数
#define  USE_GRAY_DETECT     (1)              // 0：不开启灰度显示色块 1：开启灰度显示色块
#define  SHOW_MODEL_ROI      (0)              // 0：不显示模型识别ROI 1：显示模型识别ROI 仅在彩色图像传输模式下有效
// #define  SERVER_IP           "192.168.43.7"   // TCP服务端的IP地址
#define  SERVER_IP           "192.168.43.26"   // TCP服务端的IP地址
// #define  SERVER_IP           "192.168.172.144"   // TCP服务端的IP地址
#define  PORT                8080             // TCP通信端口号
/****************************** device initial ******************************/
#define  BEEP_PATH           ZF_GPIO_BEEP     // 蜂鸣器引脚
#define  BOMA_1_PATH         ZF_GPIO_BOMA_1    // 按键引脚
#define  BOMA_2_PATH         ZF_GPIO_BOMA_2    // 按键引脚
/****************************** encoder ******************************/
#define  ENCODER_DIR_1_PATH  ZF_ENCODER_DIR_1   // 编码器方向引脚1
#define  ENCODER_DIR_2_PATH  ZF_ENCODER_DIR_2   // 编码器方向引脚2
/****************************** motor ******************************/
#define  PWM_1_PATH          ZF_PWM_MOTOR_1     // 电机1引脚
#define  DIR_1_PATH          ZF_GPIO_MOTOR_1    // 电机方向1引脚
#define  PWM_2_PATH          ZF_PWM_MOTOR_2     // 电机2引脚
#define  DIR_2_PATH          ZF_GPIO_MOTOR_2    // 电机方向2引脚
#define  ESC_PATH            ZF_PWM_ESC_1       // 电调ESC引脚
/****************************** model initial ******************************/
#define  MODEL_SELECTION            (2)         // 模型选择 1-启用TFLITE 2-启用NCNN
#if MODEL_SELECTION == 1
#define Active_classifier TFLiteclassifier
#elif MODEL_SELECTION == 2
#define Active_classifier NCNNclassifier
#endif

/****************************** device initial ******************************/
extern zf_device_ips200 ips200;
extern zf_device_uvc uvc_dev;
extern zf_driver_tcp_client tcp_client_dev;
extern zf_device_imu imu_dev;
extern zf_driver_pit pit_timer;
extern zf_device_dl1x dl1x_dev;
extern enum dl1x_device_type_enum dl1x_dev_type;
extern volatile int16_t dl1x_distance;
extern pwm_info drv8701e_pwm_1_info;
extern pwm_info drv8701e_pwm_2_info;
extern pwm_info esc_info;
extern bool system_self_lock;
extern bool green_detect;
class TFLiteCameraClassifier;
extern TFLiteCameraClassifier TFLiteclassifier;
class NCNNCameraClassifier;
extern NCNNCameraClassifier NCNNclassifier;

/****************************** model display ******************************/
void draw_rect_rgb565(uint16_t* buf, int16_t width, int16_t height, int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
/****************************** TCP ******************************/
uint32_t tcp_send_wrap(const uint8_t *buf, uint32_t len); // TCP发送数据
uint32_t tcp_read_wrap( uint8_t *buf, uint32_t len); // TCP接收数据
int8_t TCP_initial(void); // TCP初始化
void TCP_send_data(void); // TCP发送数据
void wifi_send_data(void); // WIFI发送数据
/****************************** device initial ******************************/
int8_t device_self_detect(void); // 自检
void beep_enable(bool enable); // 蜂鸣器使能
uint8_t key_scan(void); // 按键扫描
void esc_init(void); // 电调初始化
void esc_set_duty(uint16_t duty); // 电调设置占空比
void pit_callback(void); // PIT回调函数

#endif
