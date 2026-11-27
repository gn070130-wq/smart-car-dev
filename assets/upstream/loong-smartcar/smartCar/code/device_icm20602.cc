#include "device_icm20602.h"

float icm20602_transition_factor[2] = {4096, 16.4};                             // 转换实际值的比例

ICM20602::ICM20602 () 
{
    
}

ICM20602::~ICM20602() 
{
    if (fd >= 0) {
        close(fd);
    }
}

int ICM20602::ICM20602_init(void)
{
    fd = open(DEFAULT_DEVICE, O_RDWR);
    usleep(10000);
    if (fd < 0) {
        std::cerr << "打开imc20602失败!!!" << std::endl;
        return -1;
    } else {
        std::cout << "imc20602初始化完毕" << std::endl;
        float gyro_x = 0;           // 陀螺仪 x         (单位为 °/s)
        float gyro_y = 0;           // 陀螺仪 y         (单位为 °/s)
        float gyro_z = 0;           // 陀螺仪 z         (单位为 °/s)

        float accel_x = 0;          // 加速度计 x       (单位为 g(m/s^2))
        float accel_y = 0;          // 加速度计 y       (单位为 g(m/s^2))
        float accel_z = 0;          // 加速度计 z       (单位为 g(m/s^2))
    }
    // ICM20602_ACCEL_CONFIG 寄存器     需要和驱动配置相同
    // 设置为 0x00 加速度计量程为 ±2  g   获取到的加速度计数据除以 16384  可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x08 加速度计量程为 ±4  g   获取到的加速度计数据除以 8192   可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x10 加速度计量程为 ±8  g   获取到的加速度计数据除以 4096   可以转化为带物理单位的数据 单位 g(m/s^2)
    // 设置为 0x18 加速度计量程为 ±16 g   获取到的加速度计数据除以 2048   可以转化为带物理单位的数据 单位 g(m/s^2)
    switch(ICM20602_ACC_SAMPLE_DEFAULT)
    {
        case ICM20602_ACC_SAMPLE_SGN_2G:
        {
            icm20602_transition_factor[0] = 16384;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_4G:
        {
            icm20602_transition_factor[0] = 8192;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_8G:
        {
            icm20602_transition_factor[0] = 4096;
        }break;
        case ICM20602_ACC_SAMPLE_SGN_16G:
        {
            icm20602_transition_factor[0] = 2048;
        }break;
        default:
        break;
    }

    // ICM20602_GYRO_CONFIG 寄存器          需要和驱动配置相同
    // 设置为 0x00 陀螺仪量程为 ±250  dps    获取到的陀螺仪数据除以 131     可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x08 陀螺仪量程为 ±500  dps    获取到的陀螺仪数据除以 65.5    可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x10 陀螺仪量程为 ±1000 dps    获取到的陀螺仪数据除以 32.8    可以转化为带物理单位的数据 单位为 °/s
    // 设置为 0x18 陀螺仪量程为 ±2000 dps    获取到的陀螺仪数据除以 16.4    可以转化为带物理单位的数据 单位为 °/s
    switch(ICM20602_GYRO_SAMPLE_DEFAULT)
    {
        case ICM20602_GYRO_SAMPLE_SGN_250DPS:
        {
            icm20602_transition_factor[1] = 131.0;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_500DPS:
        {
            icm20602_transition_factor[1] = 65.5;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_1000DPS:
        {
            icm20602_transition_factor[1] = 32.8;
        }break;
        case ICM20602_GYRO_SAMPLE_SGN_2000DPS:
        {
            icm20602_transition_factor[1] = 16.4;
        }break;
        default:
        break;
    }
    return 0;
}

// 读取加速度计数据
void ICM20602::upDataAcc(void) 
{
    struct imu_accel_data data;
    ioctl(fd, IMU_GET_ACCEL, &data);
    accel_x = icm20602_acc_transition(data.x);
    accel_y = icm20602_acc_transition(data.y);
    accel_z = icm20602_acc_transition(data.z);
}

// 读取陀螺仪数据
void ICM20602::upDataGyro(void) 
{
    struct imu_gyro_data data;
    ioctl(fd, IMU_GET_GYRO, &data);
    gyro_x = icm20602_gyro_transition(data.x);

    gyro_y = icm20602_gyro_transition(data.y);
    gyro_y = (fabs(gyro_y) < 0.01) ? 0 : gyro_y;

    gyro_z = icm20602_gyro_transition(data.z);
    gyro_z = (fabs(gyro_z) < 0.2) ? 0 : gyro_z;
}
