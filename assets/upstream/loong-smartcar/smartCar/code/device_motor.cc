#include "device_motor.h"

Motor::Motor(void){}
Motor::~Motor(void)
{
    close(fd_motor);
}

int Motor::Motor_init(void)
{
    fd_motor = open(DEVICE_MOTOR_ADDR, O_RDWR);
    if(fd_motor < 0) {
        perror("Error open Motor\r\n");
        return -1;
    } else {
        std::cout << "电机初始化成功" << std::endl;
    }
    return 0;
}

void Motor::set_Lmotor_pid(int kp, int ki, int kd)
{
    int err;
    struct set_pid pid;
    pid.kp = kp;
    pid.ki = ki;
    pid.kd = kd;
    err = ioctl(fd_motor, MOTOR_SET_LPID, &pid);
    if(err < 0) {
        perror("Failed to init pid via ioctl\r\n");
        return;
    }
}

void Motor::set_Rmotor_pid(int kp, int ki, int kd)
{
    int err;
    struct set_pid pid;
    pid.kp = kp;
    pid.ki = ki;
    pid.kd = kd;
    err = ioctl(fd_motor, MOTOR_SET_RPID, &pid);
    if(err < 0) {
        perror("Failed to init pid via ioctl\r\n");
        return;
    }
}

void Motor:: setSpeed(int ltarget, int rtarget)
{
    int err;
    struct encoder_target encoder;
    encoder.L_encoder_target = ltarget;
    encoder.R_encoder_target = rtarget;
    err = ioctl(fd_motor, MOTOR_SET_SPEED, &encoder);
    if(err < 0) {
        perror("Failed to set encoder target via ioctl\r\n");
        return;
    }
}

void Motor:: getSpeed(void) 
{
    int err;
    struct encoder_counts encoder;
    err = ioctl(fd_motor, MOTOR_GET_SPEED, &encoder);
    if(err < 0) {
        perror("Failed to get encoder counts via ioctl\r\n");
        return;
    }
    LencoderCount = encoder.L_encoder_counts;
    RencoderCount = encoder.R_encoder_counts;
}