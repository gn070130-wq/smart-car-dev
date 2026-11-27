#ifndef CONTROL_H
#define CONTROL_H

// 误差计算
int error_compute();

int low_pass_filter_int(int new_speed, float alpha);

void speed_control(int16 left_speed, int16 right_speed, int left_goal, int right_goal);

void speed_control_reset();

int kalman_filter_int(int measurement);

void stop_car();

#endif // CONTROL_H