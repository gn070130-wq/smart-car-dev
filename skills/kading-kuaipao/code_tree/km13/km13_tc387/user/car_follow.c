#include "car_follow.h"

#pragma section all "cpu0_dsram"

static float follow_speed_target_mps = 0.0f;
static float follow_steer_target_deg = 0.0f;
static float follow_speed_integral = 0.0f;

static float follow_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float follow_limitf(float value, float min_value, float max_value)
{
    if(value < min_value) return min_value;
    if(value > max_value) return max_value;
    return value;
}

void car_follow_reset(void)
{
    follow_speed_target_mps = 0.0f;
    follow_steer_target_deg = 0.0f;
    follow_speed_integral = 0.0f;
}

void car_follow_update(const car_follow_input_struct *input,
        float measured_speed_mps, car_follow_output_struct *output)
{
    float requested_speed;
    float speed_delta;
    float speed_error;
    float pwm;

    if((NULL == input) || (NULL == output)) return;

    if(!input->valid || (input->age_ms > CAR_FOLLOW_COMMAND_TIMEOUT_MS))
    {
        follow_speed_target_mps = 0.0f;
        follow_speed_integral = 0.0f;
        output->rear_pwm = 0;
        output->steer_deg = follow_steer_target_deg;
        output->target_speed_mps = 0.0f;
        output->driving = 0U;
        return;
    }

    requested_speed = follow_limitf(input->speed_mps,
            CAR_VISION_SPEED_MIN_MPS, CAR_VISION_SPEED_MAX_MPS);
    if(requested_speed < CAR_FOLLOW_SPEED_DEADBAND_MPS)
    {
        follow_speed_target_mps = 0.0f;
        follow_speed_integral = 0.0f;
    }
    else
    {
        speed_delta = requested_speed - follow_speed_target_mps;
        speed_delta = follow_limitf(speed_delta,
                -CAR_FOLLOW_SPEED_SLEW_MPS_TICK, CAR_FOLLOW_SPEED_SLEW_MPS_TICK);
        follow_speed_target_mps += speed_delta;
    }

    if(follow_absf(input->steer_deg) <= 0.01f)
    {
        follow_steer_target_deg = 0.0f;
    }
    else if(follow_absf(input->steer_deg)
            >= (CAR_VISION_STEER_MAX_DEG - 0.1f))
    {
        // Large visual error means imminent obstacle contact. Command the
        // calibrated soft limit immediately; the raw position loop remains closed.
        follow_steer_target_deg = follow_limitf(input->steer_deg,
                -CAR_VISION_STEER_MAX_DEG, CAR_VISION_STEER_MAX_DEG);
    }
    else
    {
        follow_steer_target_deg += (follow_limitf(input->steer_deg,
                -CAR_VISION_STEER_MAX_DEG, CAR_VISION_STEER_MAX_DEG)
                - follow_steer_target_deg) * CAR_FOLLOW_STEER_FILTER_ALPHA;
    }

    if(follow_speed_target_mps <= 0.0f)
    {
        output->rear_pwm = 0;
        output->steer_deg = follow_steer_target_deg;
        output->target_speed_mps = 0.0f;
        output->driving = 0U;
        return;
    }

    speed_error = follow_speed_target_mps - measured_speed_mps;
    follow_speed_integral += speed_error * CAR_FOLLOW_SPEED_KI * CAR_CONTROL_PERIOD_S;
    follow_speed_integral = follow_limitf(follow_speed_integral,
            -CAR_FOLLOW_SPEED_I_LIMIT, CAR_FOLLOW_SPEED_I_LIMIT);
    pwm = follow_speed_target_mps * CAR_FOLLOW_PWM_PER_MPS
            + speed_error * CAR_FOLLOW_SPEED_KP + follow_speed_integral;
    if((follow_speed_target_mps >= CAR_FOLLOW_PWM_MIN_MOVE_SPEED_MPS)
            && (pwm > 0.0f) && (pwm < CAR_FOLLOW_PWM_MIN_MOVE))
    {
        pwm = CAR_FOLLOW_PWM_MIN_MOVE;
    }
    pwm = follow_limitf(pwm, 0.0f, (float)CAR_REAR_DUTY_LIMIT);

    output->rear_pwm = (int16)pwm;
    output->steer_deg = follow_steer_target_deg;
    output->target_speed_mps = follow_speed_target_mps;
    output->driving = (follow_absf(follow_speed_target_mps) > 0.0f) ? 1U : 0U;
}

#pragma section all restore
