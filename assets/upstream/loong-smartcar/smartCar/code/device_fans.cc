#include "device_fans.h"

Motor_fans::Motor_fans(void){}

Motor_fans::~Motor_fans(void)
{
    close(fd_fans);
}

int Motor_fans::Motor_fans_init(void)
{
    fd_fans = open(FANS_DIR, O_WRONLY);
    if(fd_fans < 0) {
        cerr << "Error open Fans" << endl;
        return -1;
    } else {
        cout << "无刷电机初始化成功" << endl;
    }
    return 0;
}

void Motor_fans::fans_set_duty(int dutyL, int dutyR)
{
    struct Fans_duty fans;
    fans.dutyl = dutyL;
    fans.dutyr = dutyR;
    int err = ioctl(fd_fans, FANS_SET_DUTY, &fans);
    if(err < 0) {
        perror("Failed to set fans duty via ioctl\r\n");
    }
}