#ifndef MY_GPIO_H
#define MY_GPIO_H

#include <cstdio>    // 必须：处理 FILE, fopen, fprintf, snprintf, perror
#include <iostream>  // 推荐：支持 C++ 标准流和 namespace std
#include <cstring>

// GPIO硬件编号
#define LEFT_WHELL_GPIO    15      // 左轮
#define RIGHT_WHELL_GPIO   13      // 右轮
#define ENTER_GPIO         52      // 确认
#define ESC_GPIO           16      // 退出
#define UP_GPIO            37      // 上
#define DOWN_GPIO          44      // 下
#define LEFT_GPIO          46      // 左
#define RIGHT_GPIO         53      // 右

#define LEFT_FORWARD       1
#define LEFT_BACKWARD      0
#define RIGHT_FORWARD      0
#define RIGHT_BACKWARD     1

// GPIO 初始化
void gpio_init(int gpio_num, const char *direction, int power);

// GPIO 设置输出
void gpio_set(int gpio_num, int power);

// GPIO 获取输入
int gpio_get(int gpio_num);

#endif // MY_GPIO_H
