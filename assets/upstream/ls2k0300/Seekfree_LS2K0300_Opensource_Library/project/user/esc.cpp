#include "esc.hpp"
#include "zf_common_headfile.hpp"

#include <stdio.h>
#include <string.h>

static zf_driver_pwm esc_pwm(ZF_PWM_ESC_1);
static struct pwm_info esc_pwm_info;
static float esc_current_duty_percent = ESC_MIN_DUTY_PERCENT;

static float esc_limit_duty_percent(float duty_percent)
{
    if(duty_percent < ESC_MIN_DUTY_PERCENT)
    {
        return ESC_MIN_DUTY_PERCENT;
    }
    if(duty_percent > ESC_MAX_DUTY_PERCENT)
    {
        return ESC_MAX_DUTY_PERCENT;
    }
    return duty_percent;
}

static uint16 esc_duty_percent_to_duty(float duty_percent)
{
    if(0 == esc_pwm_info.duty_max)
    {
        return 0;
    }

    float duty = duty_percent * (float)esc_pwm_info.duty_max / 100.0f;
    if(duty < 0.0f)
    {
        duty = 0.0f;
    }
    if(duty > (float)esc_pwm_info.duty_max)
    {
        duty = (float)esc_pwm_info.duty_max;
    }
    if(duty > (float)UINT16_MAX)
    {
        duty = (float)UINT16_MAX;
    }
    return (uint16)(duty + 0.5f);
}

void esc_set_duty_percent(float duty_percent)
{
    duty_percent = esc_limit_duty_percent(duty_percent);
    esc_pwm.set_duty(esc_duty_percent_to_duty(duty_percent));
    esc_current_duty_percent = duty_percent;
}

void esc_set_speed_percent(uint8 percent)
{
    if(percent > 100)
    {
        percent = 100;
    }

    float duty_percent = ESC_MIN_DUTY_PERCENT
                       + (ESC_MAX_DUTY_PERCENT - ESC_MIN_DUTY_PERCENT) * (float)percent / 100.0f;
    esc_set_duty_percent(duty_percent);
}

void esc_init(void)
{
    memset(&esc_pwm_info, 0, sizeof(esc_pwm_info));
    esc_pwm.get_dev_info(&esc_pwm_info);

    printf("esc pwm info: freq=%u duty_max=%u duty_ns=%u period_ns=%u\r\n",
           esc_pwm_info.freq,
           esc_pwm_info.duty_max,
           esc_pwm_info.duty_ns,
           esc_pwm_info.period_ns);

    esc_set_duty_percent(ESC_MIN_DUTY_PERCENT);
    system_delay_ms(ESC_ARM_TIME_MS);

    float target_duty_percent = esc_limit_duty_percent(ESC_INIT_TARGET_DUTY_PERCENT);
    if(target_duty_percent > ESC_MIN_DUTY_PERCENT)
    {
        float step_duty_percent = ESC_INIT_RAMP_STEP_DUTY_PERCENT;
        if(step_duty_percent <= 0.0f)
        {
            step_duty_percent = target_duty_percent - ESC_MIN_DUTY_PERCENT;
        }

        for(float duty_percent = ESC_MIN_DUTY_PERCENT;
            duty_percent <= target_duty_percent;
            duty_percent += step_duty_percent)
        {
            esc_set_duty_percent(duty_percent);
            system_delay_ms(ESC_INIT_RAMP_DELAY_MS);
        }
        esc_set_duty_percent(target_duty_percent);
    }

    printf("esc init done, current duty=%.2f%%\r\n", esc_current_duty_percent);
}

void esc_stop(void)
{
    esc_set_duty_percent(ESC_MIN_DUTY_PERCENT);
}

float esc_get_current_duty_percent(void)
{
    return esc_current_duty_percent;
}
