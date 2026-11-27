#ifndef _car_imu_h_
#define _car_imu_h_

#include "car_config.h"

extern volatile uint8 car_imu_ok;
extern volatile uint8 car_imu_who_am_i;
extern volatile int16 car_imu_acc_raw[3];
extern volatile int16 car_imu_gyro_raw[3];
extern volatile float car_imu_acc_g[3];
extern volatile float car_imu_gyro_dps[3];

uint8       car_imu_init                 (void);
uint8       car_imu_update               (void);
float       car_imu_forward_acc_g        (void);
float       car_imu_yaw_rate_dps         (void);
const char *car_imu_name                 (void);

#endif
