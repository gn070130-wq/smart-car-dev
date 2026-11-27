#ifndef __CONTROLTHREAD_H_
#define __CONTROLTHREAD_H_
#include "headfile.h"
#include "device_icm20602.h"
#include "device_motor.h"
#include "device_servo.h"


//智能车速度
extern int smartCar_speed;
//电机使能
extern bool motor_enable;
//负压使能
extern bool fans_enable;
//电机启动倒计时
extern int motor_enable_time;
//使能电机使能
extern bool motor_enable_enable;
//陀螺仪z轴积分
extern float gyro_z_integration;
//陀螺仪x轴积分
extern float gyro_y_integration;
//编码器积分
extern int motor_distance;
//控制锁
extern std::mutex control_mutex;
//电机使能锁
extern std::mutex motor_enable_mutex;
//负压激增
extern int fans_time_duty;
//跟踪距离
extern int trackAim;

void controlThread_Star(void);
//电机使能
void motor_enable_off(void);
//电机失能
void motor_enable_on(void);
//使能电机使能
void motor_enable_star(int ms);
//清除陀螺仪积分
void gyro_integration_clean(void);
//清除编码器积分
void motor_distance_cleam(void);

#endif
