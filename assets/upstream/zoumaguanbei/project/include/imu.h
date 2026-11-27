#ifndef IMU_H
#define IMU_H

void update_quaternion_from_imu();

float get_euler_angle(int axis);
    
#endif // IMU_H
