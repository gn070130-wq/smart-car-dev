#ifndef __ESC_HPP__
#define __ESC_HPP__

#include "zf_common_typedef.hpp"

// 默认按 50Hz 电调信号配置：20ms 一个周期。
// 0% 油门=5.0%占空比，70%油门=8.5%占空比，100%油门=10.0%占空比。
#define ESC_MIN_DUTY_PERCENT                (5.0f)
#define ESC_INIT_TARGET_DUTY_PERCENT        (10.0f)
#define ESC_MAX_DUTY_PERCENT                (10.0f)
#define ESC_ARM_TIME_MS                     (2000U)
#define ESC_INIT_RAMP_STEP_DUTY_PERCENT     (0.05f)
#define ESC_INIT_RAMP_DELAY_MS              (20U)

void esc_init(void);
void esc_stop(void);
void esc_set_duty_percent(float duty_percent);
void esc_set_speed_percent(uint8 percent);
float esc_get_current_duty_percent(void);

#endif
