#ifndef __CHASSIS_HPP__
#define __CHASSIS_HPP__

#include "vision.hpp"
#include "zf_common_headfile.hpp"

//PID调控函数
void chassis_init(void);    //底盘初始化函数
void chassis_gyro_control_tick(void);    //底盘陀螺仪中环控制函数，放入定时器，每5ms调控一次
void chassis_speed_control_tick(void);   //底盘速度内环控制函数，放入定时器，每5ms调控一次
void chassis_stop(void);    //底盘停止函数
void chassis_vision_control_tick(VisionResult &vision_result);
bool chassis_red_avoidance_encoder_reached(int32 target_count); // 判断绕行编码器平均计数是否到达目标值

//底盘上位机函数
void chassis_send_debug_waveform(const VisionResult &vision_result);  // 发送逐飞助手示波器波形数据
void chassis_update_vision_pid_from_assistant(void); // 从逐飞助手参数通道更新底盘控制参数

//底盘陀螺仪函数
void yaw_offset_calibration(void); // 陀螺仪Z轴零偏校准，需在水平静止状态下调用
void yaw_start_cout(uint8_t state); // 控制圆环偏航角是否开始积分
float yaw_get_from_imu(void); // 获取圆环状态机使用的偏航角积分值


#endif
