#include "car_imu.h"

#pragma section all "cpu0_dsram"

#define CAR_ICM45686_REG_ACCEL_DATA      (0x00)
#define CAR_ICM45686_REG_PWR_MGMT0       (0x10)
#define CAR_ICM45686_REG_ACCEL_CONFIG0   (0x1B)
#define CAR_ICM45686_REG_GYRO_CONFIG0    (0x1C)
#define CAR_ICM45686_REG_INT1_STATUS0    (0x19)
#define CAR_ICM45686_REG_WHO_AM_I        (0x72)
#define CAR_ICM45686_REG_IREG_ADDR_H     (0x7C)
#define CAR_ICM45686_REG_IREG_DATA       (0x7E)
#define CAR_ICM45686_REG_MISC2           (0x7F)
#define CAR_ICM45686_MREG_SREG_CTRL      (0xA267)
#define CAR_ICM45686_SOFT_RESET          (0x02)
#define CAR_ICM45686_RESET_DONE          (0x80)
#define CAR_ICM45686_ENDIAN_BIG          (0x02)

volatile uint8 car_imu_ok = 0;
volatile uint8 car_imu_who_am_i = 0;
volatile int16 car_imu_acc_raw[3] = {0, 0, 0};
volatile int16 car_imu_gyro_raw[3] = {0, 0, 0};
volatile float car_imu_acc_g[3] = {0.0f, 0.0f, 0.0f};
volatile float car_imu_gyro_dps[3] = {0.0f, 0.0f, 0.0f};

#if CAR_IMU_TYPE == CAR_IMU_TYPE_ICM45686
static soft_iic_info_struct car_icm45686_iic;
static uint8 car_icm45686_big_endian = 1;

static uint8 car_icm45686_read_register(uint8 reg)
{
    return soft_iic_read_8bit_register(&car_icm45686_iic, reg);
}

static void car_icm45686_write_register(uint8 reg, uint8 data)
{
    soft_iic_write_8bit_register(&car_icm45686_iic, reg, data);
}

static uint8 car_icm45686_read_mreg(uint16 reg)
{
    uint8 address[2];

    address[0] = (uint8)(reg >> 8);
    address[1] = (uint8)reg;
    system_delay_us(4);
    soft_iic_write_8bit_registers(&car_icm45686_iic,
            CAR_ICM45686_REG_IREG_ADDR_H, address, 2);
    system_delay_us(4);
    return car_icm45686_read_register(CAR_ICM45686_REG_IREG_DATA);
}

static int16 car_icm45686_parse_i16(const uint8 *data)
{
    uint16 value;

    if(car_icm45686_big_endian)
    {
        value = ((uint16)data[0] << 8) | data[1];
    }
    else
    {
        value = ((uint16)data[1] << 8) | data[0];
    }
    return (int16)value;
}

static uint8 car_icm45686_init(void)
{
    uint8 reset_status;
    uint8 accel_config;
    uint8 gyro_config;
    uint8 pwr_config;
    uint8 sreg_ctrl;

    soft_iic_init(&car_icm45686_iic, CAR_ICM45686_IIC_ADDR,
            CAR_ICM45686_IIC_DELAY, CAR_ICM45686_SCL_PIN, CAR_ICM45686_SDA_PIN);
    system_delay_ms(10);

    // The first I2C transaction after power-up may NACK; perform the documented retry.
    car_imu_who_am_i = car_icm45686_read_register(CAR_ICM45686_REG_WHO_AM_I);
    system_delay_ms(1);
    car_imu_who_am_i = car_icm45686_read_register(CAR_ICM45686_REG_WHO_AM_I);
    if(CAR_ICM45686_WHO_AM_I_VALUE != car_imu_who_am_i)
    {
        return 1;
    }

    car_icm45686_write_register(CAR_ICM45686_REG_MISC2, CAR_ICM45686_SOFT_RESET);
    system_delay_ms(2);
    reset_status = car_icm45686_read_register(CAR_ICM45686_REG_INT1_STATUS0);
    car_imu_who_am_i = car_icm45686_read_register(CAR_ICM45686_REG_WHO_AM_I);
    if((CAR_ICM45686_WHO_AM_I_VALUE != car_imu_who_am_i)
            || (0 == (reset_status & CAR_ICM45686_RESET_DONE)))
    {
        return 1;
    }

    car_icm45686_write_register(CAR_ICM45686_REG_ACCEL_CONFIG0,
            CAR_ICM45686_ACCEL_CONFIG);
    car_icm45686_write_register(CAR_ICM45686_REG_GYRO_CONFIG0,
            CAR_ICM45686_GYRO_CONFIG);
    car_icm45686_write_register(CAR_ICM45686_REG_PWR_MGMT0,
            CAR_ICM45686_PWR_CONFIG);
    system_delay_ms(CAR_ICM45686_STARTUP_MS);

    accel_config = car_icm45686_read_register(CAR_ICM45686_REG_ACCEL_CONFIG0);
    gyro_config = car_icm45686_read_register(CAR_ICM45686_REG_GYRO_CONFIG0);
    pwr_config = car_icm45686_read_register(CAR_ICM45686_REG_PWR_MGMT0);
    if((CAR_ICM45686_ACCEL_CONFIG != accel_config)
            || (CAR_ICM45686_GYRO_CONFIG != gyro_config)
            || (CAR_ICM45686_PWR_CONFIG != (pwr_config & 0x0F)))
    {
        return 1;
    }

    sreg_ctrl = car_icm45686_read_mreg(CAR_ICM45686_MREG_SREG_CTRL);
    car_icm45686_big_endian = (sreg_ctrl & CAR_ICM45686_ENDIAN_BIG) ? 1 : 0;
    return 0;
}

static uint8 car_icm45686_update(void)
{
    uint8 data[12];
    uint8 i;
    uint8 nonzero = 0;
    uint8 not_ff = 0;
    float acc_scale = 32768.0f / CAR_ICM45686_ACCEL_FS_G;
    float gyro_scale = 32768.0f / CAR_ICM45686_GYRO_FS_DPS;

    soft_iic_read_8bit_registers(&car_icm45686_iic,
            CAR_ICM45686_REG_ACCEL_DATA, data, 12);
    for(i = 0; i < 12; i++)
    {
        nonzero |= data[i];
        not_ff |= (uint8)(data[i] ^ 0xFF);
    }
    if((0 == nonzero) || (0 == not_ff))
    {
        return 1;
    }

    for(i = 0; i < 3; i++)
    {
        car_imu_acc_raw[i] = car_icm45686_parse_i16(&data[i * 2]);
        car_imu_gyro_raw[i] = car_icm45686_parse_i16(&data[6 + i * 2]);
        car_imu_acc_g[i] = (float)car_imu_acc_raw[i] / acc_scale;
        car_imu_gyro_dps[i] = (float)car_imu_gyro_raw[i] / gyro_scale;
    }
    return 0;
}
#endif

uint8 car_imu_init(void)
{
    uint8 state;

#if CAR_IMU_TYPE == CAR_IMU_TYPE_ICM45686
    state = car_icm45686_init();
#elif CAR_IMU_TYPE == CAR_IMU_TYPE_MPU6050
    state = mpu6050_init();
    car_imu_who_am_i = state ? 0 : 0x68;
#elif CAR_IMU_TYPE == CAR_IMU_TYPE_IMU963RA
    state = imu963ra_init();
    car_imu_who_am_i = state ? 0 : IMU963RA_DEV_ADDR;
#else
#error "Unsupported CAR_IMU_TYPE"
#endif

    car_imu_ok = state ? 0 : 1;
    if(car_imu_ok)
    {
        state = car_imu_update();
        car_imu_ok = state ? 0 : 1;
    }
    return car_imu_ok ? 0 : 1;
}

uint8 car_imu_update(void)
{
    uint8 state = 0;

#if CAR_IMU_TYPE == CAR_IMU_TYPE_ICM45686
    state = car_icm45686_update();
#elif CAR_IMU_TYPE == CAR_IMU_TYPE_MPU6050
    mpu6050_get_acc();
    mpu6050_get_gyro();
    car_imu_acc_raw[0] = mpu6050_acc_x;
    car_imu_acc_raw[1] = mpu6050_acc_y;
    car_imu_acc_raw[2] = mpu6050_acc_z;
    car_imu_gyro_raw[0] = mpu6050_gyro_x;
    car_imu_gyro_raw[1] = mpu6050_gyro_y;
    car_imu_gyro_raw[2] = mpu6050_gyro_z;
    car_imu_acc_g[0] = mpu6050_acc_transition(car_imu_acc_raw[0]);
    car_imu_acc_g[1] = mpu6050_acc_transition(car_imu_acc_raw[1]);
    car_imu_acc_g[2] = mpu6050_acc_transition(car_imu_acc_raw[2]);
    car_imu_gyro_dps[0] = mpu6050_gyro_transition(car_imu_gyro_raw[0]);
    car_imu_gyro_dps[1] = mpu6050_gyro_transition(car_imu_gyro_raw[1]);
    car_imu_gyro_dps[2] = mpu6050_gyro_transition(car_imu_gyro_raw[2]);
#elif CAR_IMU_TYPE == CAR_IMU_TYPE_IMU963RA
    imu963ra_get_acc();
    imu963ra_get_gyro();
    car_imu_acc_raw[0] = imu963ra_acc_x;
    car_imu_acc_raw[1] = imu963ra_acc_y;
    car_imu_acc_raw[2] = imu963ra_acc_z;
    car_imu_gyro_raw[0] = imu963ra_gyro_x;
    car_imu_gyro_raw[1] = imu963ra_gyro_y;
    car_imu_gyro_raw[2] = imu963ra_gyro_z;
    car_imu_acc_g[0] = imu963ra_acc_transition(car_imu_acc_raw[0]);
    car_imu_acc_g[1] = imu963ra_acc_transition(car_imu_acc_raw[1]);
    car_imu_acc_g[2] = imu963ra_acc_transition(car_imu_acc_raw[2]);
    car_imu_gyro_dps[0] = imu963ra_gyro_transition(car_imu_gyro_raw[0]);
    car_imu_gyro_dps[1] = imu963ra_gyro_transition(car_imu_gyro_raw[1]);
    car_imu_gyro_dps[2] = imu963ra_gyro_transition(car_imu_gyro_raw[2]);
#endif

    car_imu_ok = state ? 0 : 1;
    return state;
}

float car_imu_forward_acc_g(void)
{
    return car_imu_acc_g[CAR_IMU_FORWARD_AXIS] * CAR_IMU_FORWARD_SIGN;
}

float car_imu_yaw_rate_dps(void)
{
    return car_imu_gyro_dps[CAR_IMU_YAW_AXIS] * CAR_IMU_YAW_SIGN;
}

const char *car_imu_name(void)
{
#if CAR_IMU_TYPE == CAR_IMU_TYPE_ICM45686
    return "ICM45686";
#elif CAR_IMU_TYPE == CAR_IMU_TYPE_IMU963RA
    return "IMU963RA";
#else
    return "MPU6050";
#endif
}

#pragma section all restore
