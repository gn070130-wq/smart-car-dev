#include "device_servo.h"

/*  初始化舵机频率和占空比
 *
 *  freq : 频率
 *  duty : 占空比
 */
Servo::Servo(void) : duty(duty){}

Servo::~Servo(void)
{
    close(fd_servo);
}

int Servo::Servo_init(int freq, int duty)
{
    fd_servo = open(DEVICE_SERVO_ADDR, O_WRONLY);
    if(fd_servo < 0) {
        perror("Error open Servo\r\n");
        return -1;
    } else {
        set_freq(freq);
        set_duty(duty);
        std::cout << "舵机初始化成功" << std::endl;
    }
    return 0;
}


void Servo::set_servo(int error)
{
    error = (error > SERVO_MAXERROR) ? SERVO_MAXERROR : (error < -SERVO_MAXERROR) ? -SERVO_MAXERROR : error;

    set_duty(preprocess.servoMID+error);
}


void Servo::set_duty(int value)
{
    int err;
    duty = value;
    err = ioctl(fd_servo, SERVO_SET_DUTY, &duty);
    if(err < 0) {
        perror("Failed to set servo duty via ioctl\r\n");
        return;
    }
}

void Servo::set_freq(int freq)
{
    int err = ioctl(fd_servo, SERVO_SET_FREQ, &freq);
    if(err < 0) {
        perror("Failed to set servo duty via ioctl\r\n");
        return;
    }
}
