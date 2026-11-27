#include "zf_common_headfile.h"

int accel_num[3] = {0};
int gyro_num[3] = {0};

static float q0 = 1.0f, q1 = 0.0f, q2 = 0.0f, q3 = 0.0f;

// 欧拉角（单位：度）
static float roll, pitch, yaw;

// 时间步长（单位：秒）
#define DT 0.02f

// 滤波器增益
#define Kp 5.0f

// 更新四元数（自动读取 IMU）
void update_quaternion_from_imu() {
    // 获取原始数据
    imu_acc_x = imu_get_raw(imu_file_path[ACC_X_RAW]) / 10 * 10;
    imu_acc_y = imu_get_raw(imu_file_path[ACC_Y_RAW]) / 10 * 10;
    imu_acc_z = imu_get_raw(imu_file_path[ACC_Z_RAW]) / 10 * 10;
    imu_gyro_x = imu_get_raw(imu_file_path[GYRO_X_RAW]) / 10 * 10;
    imu_gyro_y = imu_get_raw(imu_file_path[GYRO_Y_RAW]) / 10 * 10;
    imu_gyro_z = imu_get_raw(imu_file_path[GYRO_Z_RAW]) / 10 * 10;

    // 单位换算（根据你的 IMU 规格）
    float ax = imu_acc_x / 16384.0f;
    float ay = imu_acc_y / 16384.0f;
    float az = imu_acc_z / 16384.0f;

    float gx = imu_gyro_x / 131.0f * (M_PI / 180.0f);
    float gy = imu_gyro_y / 131.0f * (M_PI / 180.0f);
    float gz = imu_gyro_z / 131.0f * (M_PI / 180.0f);

    // 归一化加速度
    float norm = sqrtf(ax*ax + ay*ay + az*az);
    if (norm == 0.0f) return;
    ax /= norm;
    ay /= norm;
    az /= norm;

    // 重力估计
    float vx = 2*(q1*q3 - q0*q2);
    float vy = 2*(q0*q1 + q2*q3);
    float vz = q0*q0 - q1*q1 - q2*q2 + q3*q3;

    // 误差项
    float ex = (ay*vz - az*vy);
    float ey = (az*vx - ax*vz);
    float ez = (ax*vy - ay*vx);

    // 纠正陀螺仪
    gx += Kp * ex;
    gy += Kp * ey;
    gz += Kp * ez;

    // 四元数积分
    float q0_temp = q0 + (-q1*gx - q2*gy - q3*gz) * 0.5f * DT;
    float q1_temp = q1 + ( q0*gx + q2*gz - q3*gy) * 0.5f * DT;
    float q2_temp = q2 + ( q0*gy - q1*gz + q3*gx) * 0.5f * DT;
    float q3_temp = q3 + ( q0*gz + q1*gy - q2*gx) * 0.5f * DT;

    // 归一化四元数
    norm = sqrtf(q0_temp*q0_temp + q1_temp*q1_temp + q2_temp*q2_temp + q3_temp*q3_temp);
    q0 = q0_temp / norm;
    q1 = q1_temp / norm;
    q2 = q2_temp / norm;
    q3 = q3_temp / norm;

    // 更新欧拉角
    roll  = atan2f(2*(q0*q1 + q2*q3), 1 - 2*(q1*q1 + q2*q2)) * (180.0f / M_PI);
    pitch = asinf(2*(q0*q2 - q3*q1)) * (180.0f / M_PI);
    yaw   = atan2f(2*(q0*q3 + q1*q2), 1 - 2*(q2*q2 + q3*q3)) * (180.0f / M_PI);
}

// 获取指定轴的欧拉角（单位：度）
// 0 -> Roll (X), 1 -> Pitch (Y), 2 -> Yaw (Z)
float get_euler_angle(int axis) {
    if (axis == 0) return roll;
    else if (axis == 1) return pitch;
    else if (axis == 2) return yaw;
    else return 0.0f;
}

