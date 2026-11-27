#ifndef DEVICE_MOTOR_H
#define DEVICE_MOTOR_H

#include "headfile.h"

#define DEVICE_MOTOR_ADDR   "/dev/wuwu_motor"

// 定义与驱动中相同的结构体和命令
struct encoder_target {
    int L_encoder_target;
    int R_encoder_target;
};

struct encoder_counts {
    int L_encoder_counts;
    int R_encoder_counts;
};

struct set_pid {
    int kp, ki, kd;
};

#define MOTOR_IOCTL_MAGIC 'O'
#define MOTOR_SET_SPEED    _IOW(MOTOR_IOCTL_MAGIC, 1, struct encoder_target)
#define MOTOR_GET_SPEED    _IOR(MOTOR_IOCTL_MAGIC, 2, struct encoder_counts)
#define MOTOR_SET_LPID    _IOW(MOTOR_IOCTL_MAGIC, 3, struct set_pid)
#define MOTOR_SET_RPID    _IOW(MOTOR_IOCTL_MAGIC, 4, struct set_pid)

class Motor
{
public:
    int LencoderCount = 0;
    int RencoderCount = 0;
    int Ltarget = 0;
    int Rtarget = 0;

    Motor(void);
    ~Motor(void);
    int Motor_init(void);
    void set_Lmotor_pid(int kp, int ki, int kd);
    void set_Rmotor_pid(int kp, int ki, int kd);
    void setSpeed(int ltarget, int rtarget);
    void getSpeed(void);

private:
    int fd_motor;

};

extern Motor motor;

#endif
