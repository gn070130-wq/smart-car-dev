#ifndef __MOTOR_FANS_H_
#define __MOTOR_FANS_H_

#include "headfile.h"

#define FANS_DIR    "/dev/wuwu_fans"

// 在头文件或模块顶部添加
#define FANS_IOCTL_MAGIC    'F'
#define FANS_SET_DUTY    _IOW(FANS_IOCTL_MAGIC, 1, struct Fans_duty)

struct Fans_duty
{
    int dutyl;
    int dutyr;
};

class Motor_fans 
{
public:
    int dutyL;
    int dutyR;

    Motor_fans(void);
    ~Motor_fans(void);
    int Motor_fans_init(void);
    /* fans_set_duty(dutyL, dutyR);
     * 范围0～100
     * 内核写好了限幅大胆调用
     */
    void fans_set_duty(int dutyL, int dutyR);

private:
    int fd_fans;
};

extern Motor_fans fans;

#endif
