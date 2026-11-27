#ifndef __DEVICE_SERVO_H_
#define __DEVICE_SERVO_H_

#include "headfile.h"

#define DEVICE_SERVO_ADDR   "/dev/wuwu_servo"
#define SERVO_HZ            200
#define SERVO_MID           3240
#define SERVO_MAXERROR      275

// 在头文件或模块顶部添加
#define SERVO_IOCTL_MAGIC 'S'
#define SERVO_SET_DUTY     _IOW(SERVO_IOCTL_MAGIC, 1, int)
#define SERVO_SET_FREQ     _IOW(SERVO_IOCTL_MAGIC, 2, int)

class Servo
{
public:
    int duty = 0;
    Servo();
    ~Servo(void);

    int Servo_init(int freq, int duty);
    void set_servo(int error);
    /*  设置舵机占空比
     *
     *  void set_duty(int value)
     *  value : 占空比范围（0~10000)
     */
    void set_duty(int value);
    void set_freq(int freq);
private: 
    int fd_servo;
};

extern Servo servo;

#endif
