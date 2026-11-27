#ifndef PWM_H
#define PWM_H

#include <cstdio>    // 必须：处理 FILE, fopen, fprintf, snprintf, perror
#include <iostream>  // 推荐：支持 C++ 标准流和 namespace std
#include <cstring>
#include <fstream>

// PWM硬件编号
#define SERVO              5
#define POWER_TRANSFER     4
#define LEFT_WHELL_PWM     0
#define RIGHT_WHELL_PWM    1
#define BUZZER             3

// PWM 初始化
void pwm_init(int pwm_num, int frequency, int duty_cycle);

void pwm_set_polarity(int pwm_num, const char* polarity);

// PWM 设置占空比
void pwm_set_duty(int pwm_num, int duty_cycle);

void pwm_test();

void pwm_clean();


#endif // PWM_H
