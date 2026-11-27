#include "headfile.h"

// 全局电机Motor对象
static Motor g_motor;

static inline int iclamp(int v, int lo, int hi)
{
    if (v < lo) return lo;
    if (v > hi) return hi;
    return v;
}

static int speed_to_duty(int speed)
{
    int abs_spd = abs(iclamp(speed, -100, 100));
    return abs_spd * MOTOR_MAX_DUTY / 100;
}

int car_init()
{
    if (g_motor.motor_init() < 0) {
        return -1;
    }
    g_motor.set_motor1(MOTOR_FORWARD_LEVEL, 0);
    g_motor.set_motor2(MOTOR_FORWARD_LEVEL, 0);
    return 0;
}

void car_set_speed_1(int speed)
{
    speed = iclamp(speed, -100, 100);
    int duty = speed_to_duty(speed);
    int dir  = (speed >= 0) ? MOTOR_FORWARD_LEVEL : MOTOR_BACKWARD_LEVEL;
    g_motor.set_motor1(dir, duty);
}

void car_set_speed_2(int speed)
{
    speed = iclamp(speed, -100, 100);
    int duty = speed_to_duty(speed);
    int dir  = (speed >= 0) ? MOTOR_FORWARD_LEVEL : MOTOR_BACKWARD_LEVEL;
    g_motor.set_motor2(dir, duty);
}

void car_stop()
{
    g_motor.set_motor1(MOTOR_FORWARD_LEVEL, 0);
    g_motor.set_motor2(MOTOR_FORWARD_LEVEL, 0);
}

void car_set_speed_diff(int left_speed, int right_speed)
{
    left_speed = iclamp(left_speed, -100, 100);
    right_speed = iclamp(right_speed, -100, 100);

    int left_duty  = speed_to_duty(left_speed);
    int right_duty = speed_to_duty(right_speed);

    int left_dir  = (left_speed >= 0)  ? MOTOR_FORWARD_LEVEL : MOTOR_BACKWARD_LEVEL;
    int right_dir = (right_speed >= 0) ? MOTOR_FORWARD_LEVEL : MOTOR_BACKWARD_LEVEL;

    g_motor.set_motor1(left_dir, left_duty);
    g_motor.set_motor2(right_dir, right_duty);
}

void car_drive_pid(int base_speed, int turn_value)
{
    int left  = base_speed + turn_value;
    int right = base_speed - turn_value;
    car_set_speed_diff(left, right);
}


void car_update_encoders()
{
    g_motor.update_encoders();
    g_motor.thread_syn();
}

int32_t car_get_encoder1()
{
    return g_motor.syn_encoder1_counts;
}

int32_t car_get_encoder2()
{
    return g_motor.syn_encoder2_counts;
}
