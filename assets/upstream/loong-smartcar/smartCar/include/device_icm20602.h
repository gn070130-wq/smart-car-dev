#ifndef _ICM20602_H_
#define _ICM20602_H_

#include "headfile.h"

#define DEFAULT_DEVICE    "/dev/wuwu_imc20602"
#define IMU_IOCTL_MAGIC   'i'
#define IMU_GET_ACCEL     _IOR(IMU_IOCTL_MAGIC, 1, imu_accel_data)
#define IMU_GET_GYRO      _IOR(IMU_IOCTL_MAGIC, 2, imu_gyro_data)

//向驱动访问的加速度结构体
struct imu_accel_data {
    int16_t x, y, z;
};
//向驱动访问的陀螺仪结构体
struct imu_gyro_data {
    int16_t x, y, z;
};

extern float icm20602_transition_factor[2];

typedef enum
{
    ICM20602_ACC_SAMPLE_SGN_2G ,                                                // 加速度计量程 ±2G  (ACC = Accelerometer 加速度计) (SGN = signum 带符号数 表示正负范围) (G = g 重力加速度 g≈9.80 m/s^2)
    ICM20602_ACC_SAMPLE_SGN_4G ,                                                // 加速度计量程 ±4G  (ACC = Accelerometer 加速度计) (SGN = signum 带符号数 表示正负范围) (G = g 重力加速度 g≈9.80 m/s^2)
    ICM20602_ACC_SAMPLE_SGN_8G ,                                                // 加速度计量程 ±8G  (ACC = Accelerometer 加速度计) (SGN = signum 带符号数 表示正负范围) (G = g 重力加速度 g≈9.80 m/s^2)
    ICM20602_ACC_SAMPLE_SGN_16G,                                                // 加速度计量程 ±16G (ACC = Accelerometer 加速度计) (SGN = signum 带符号数 表示正负范围) (G = g 重力加速度 g≈9.80 m/s^2)
}icm20602_acc_sample_config;

typedef enum
{
    ICM20602_GYRO_SAMPLE_SGN_250DPS ,                                           // 陀螺仪量程 ±250DPS  (GYRO = Gyroscope 陀螺仪) (SGN = signum 带符号数 表示正负范围) (DPS = Degree Per Second 角速度单位 °/S)
    ICM20602_GYRO_SAMPLE_SGN_500DPS ,                                           // 陀螺仪量程 ±500DPS  (GYRO = Gyroscope 陀螺仪) (SGN = signum 带符号数 表示正负范围) (DPS = Degree Per Second 角速度单位 °/S)
    ICM20602_GYRO_SAMPLE_SGN_1000DPS,                                           // 陀螺仪量程 ±1000DPS (GYRO = Gyroscope 陀螺仪) (SGN = signum 带符号数 表示正负范围) (DPS = Degree Per Second 角速度单位 °/S)
    ICM20602_GYRO_SAMPLE_SGN_2000DPS,                                           // 陀螺仪量程 ±2000DPS (GYRO = Gyroscope 陀螺仪) (SGN = signum 带符号数 表示正负范围) (DPS = Degree Per Second 角速度单位 °/S)
}icm20602_gyro_sample_config;

#define ICM20602_ACC_SAMPLE_DEFAULT     ( ICM20602_ACC_SAMPLE_SGN_8G )          // 在这设置默认的 加速度计 初始化量程
#define ICM20602_GYRO_SAMPLE_DEFAULT    ( ICM20602_GYRO_SAMPLE_SGN_2000DPS )    // 在这设置默认的 陀螺仪   初始化量程

//-------------------------------------------------------------------------------------------------------------------
// 函数简介     将 ICM20602 加速度计数据转换为实际物理数据
// 参数说明     acc_value       任意轴的加速度计数据
// 返回参数     void
// 使用示例     float data = icm20602_acc_transition(icm20602_acc_x);               // 单位为 g(m/s^2)
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
#define icm20602_acc_transition(acc_value)      ((float)(acc_value) / icm20602_transition_factor[0])
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     将 ICM20602 陀螺仪数据转换为实际物理数据
// 参数说明     gyro_value      任意轴的陀螺仪数据
// 返回参数     void
// 使用示例     float data = icm20602_gyro_transition(icm20602_gyro_x);             // 单位为 °/s
// 备注信息
//-------------------------------------------------------------------------------------------------------------------
#define icm20602_gyro_transition(gyro_value)    ((float)(gyro_value) / icm20602_transition_factor[1])

class ICM20602 {
public:
    float gyro_x;           // 陀螺仪 x         (单位为 °/s)
    float gyro_y;           // 陀螺仪 y         (单位为 °/s)
    float gyro_z;           // 陀螺仪 z         (单位为 °/s)

    float accel_x;          // 加速度计 x       (单位为 g(m/s^2))
    float accel_y;          // 加速度计 y       (单位为 g(m/s^2))
    float accel_z;          // 加速度计 z       (单位为 g(m/s^2))

    ICM20602(void);
    ~ICM20602(void);  
    int ICM20602_init(void);
    // 读取加速度计数据
    void upDataAcc(void);
    // 读取陀螺仪数据
    void upDataGyro(void);

private:
    int fd;  // 设备文件描述符
    
};

extern ICM20602 imc20602;

#endif
