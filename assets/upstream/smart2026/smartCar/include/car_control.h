#pragma once
#include <cstdint>

/* ── 电机初始化 ── */
int  car_init();

/* ── 单电机控制 ── */
void car_set_speed_1(int speed);     // 左电机 -100~100
void car_set_speed_2(int speed);     // 右电机 -100~100

/* ── 差速驱动 ── */
void car_set_speed_diff(int left_speed, int right_speed);

/* ── PID 循迹辅助 ── */
void car_drive_pid(int base_speed, int turn_value);

/* ── 停车 ── */
void car_stop();

/* ── 编码器（通过同一个Motor对象读取，避免设备冲突） ── */
void    car_update_encoders();
int32_t car_get_encoder1();
int32_t car_get_encoder2();
