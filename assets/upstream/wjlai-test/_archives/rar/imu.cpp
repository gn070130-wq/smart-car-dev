#include "imu.h"

// 全局积分器对象
GyroIntegrator gyro_int;

// IMU设备对象（需在main中初始化，这里仅声明为外部引用）
extern zf_device_imu imu_dev;

// 灵敏度：IMU660RA/IMU660RB/IMU963RA 默认量程 ±2000°/s，对应16.4 LSB/(°/s)
#define GYRO_SENSITIVITY    16.4f

// 死区阈值：小于此值的角速度视为噪声（°/s）
#define GYRO_DEADZONE       0.1f

//陀螺仪z、y轴积分
float gyro_z_integration;
float gyro_y_integration ;

//编码器积分
float motor_distance;


// 函数简介     初始化积分器
//-------------------------------------------------------------------------------------------------------------------
void gyro_integrator_init(void) 
{
    gyro_int.integration = 0.0f;
    gyro_int.filtered_integration = 0.0f;   // 新增
    gyro_int.last_filtered = 0.0f;          // 新增
    gyro_int.raw_offset = 0.0f;
    gyro_int.last_gyro = 0.0f;
    gyro_int.is_calibrated = false;
}
//-------------------------------------------------------------------------------------------------------------------
// 函数简介     静态零偏校准（在车辆完全静止时调用）
// 参数说明     samples 采集次数，建议100次
//-------------------------------------------------------------------------------------------------------------------
void gyro_integrator_calibrate(int samples)
{
    if(samples <= 0) return;

    int64_t sum = 0;
    for(int i = 0; i < samples; i++)
    {
        // 读取原始陀螺仪Z轴数值
        int16_t raw = imu_dev.get_gyro_z();
        sum += raw;

        // 等待10ms，保证采样间隔均匀（可选，但建议加上）
        system_delay_ms(5);
    }

    gyro_int.raw_offset = (float)sum / samples;   // 计算平均零偏
    gyro_int.is_calibrated = true;                 // 标记已校准
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     更新积分（需周期调用，例如每帧图像处理时调用一次）
//-------------------------------------------------------------------------------------------------------------------
void gyro_integrator_update(void)
{
    // 1. 读取原始值并转化为角速度 (度/秒)
    int16_t raw = imu_dev.get_gyro_z();
    float gyro_now = ((float)raw - gyro_int.raw_offset) / GYRO_SENSITIVITY;
    
    // 3. 极其关键：dt 必须和定时器周期严格一致！
    // 你的 pit_timer.init_ms(5, ...) 是 5ms，所以 dt 必须是 0.005
    float dt = 0.005f; 
    
    // 4. 使用滤波后的角速度进行梯形积分
    gyro_int.integration += (gyro_int.last_gyro + gyro_now) * 0.5f * dt;
    
    // 5. 更新历史状态
    gyro_int.last_gyro = gyro_now;
    
    // 注意：以后直接使用 gyro_int.integration 作为你的绝对角度，不需要再有个 filtered_integration 了
}


//-------------------------------------------------------------------------------------------------------------------
// 函数简介     重置累积角度（入环时清零）
//-------------------------------------------------------------------------------------------------------------------
void gyro_integration_clean(void) {
    gyro_int.integration = 0.0f;
    gyro_int.filtered_integration = 0.0f;   // 新增
    gyro_int.last_filtered = 0.0f;          // 新增
    int16_t raw = imu_dev.get_gyro_z();
    gyro_int.last_gyro = ((float)raw - gyro_int.raw_offset) / GYRO_SENSITIVITY;
}


void motor_distance_clean(void) 
{
    motor_distance = 0.0f;
}