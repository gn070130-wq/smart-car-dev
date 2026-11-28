#ifndef BOARD_HAL_H_
#define BOARD_HAL_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void hal_init(void);
uint32_t hal_millis(void);
void hal_delay_ms(uint32_t ms);

/* 电感 0..n-1 → 归一化前原始 ADC；n 由板定，常用 4~8 */
void hal_adc_inductance(float *out, int n);

/* IMU：deg / deg/s */
void hal_imu_read(float *ax, float *ay, float *az, float *gx, float *gy, float *gz);
float hal_yaw_deg(void);

/* 编码器脉冲累计或转速 */
void hal_encoder_read(int32_t *left, int32_t *right);

/* 执行器：占空比 -1..1 或 PWM 约定由实现文档说明 */
void hal_motor_set(float left, float right);
void hal_servo_set(float deg);
void hal_fan_set(float duty); /* 负压/风机，无则空实现 */

void hal_led(int on);
int hal_key_pressed(void);

#ifdef __cplusplus
}
#endif

#endif
