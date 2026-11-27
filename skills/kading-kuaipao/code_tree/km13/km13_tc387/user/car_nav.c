#include "car_nav.h"
#include "car_follow.h"
#include "car_imu.h"
#include "car_replay.h"
#include "car_vision.h"
#include "zf_device_absolute_encoder.h"
#include <stdio.h>
#include <string.h>

#pragma section all "cpu0_dsram"

volatile car_state_enum car_state = CAR_STATE_SELF_TEST;
volatile uint32 car_elapsed_ms = 0;
volatile uint8 car_nav_10ms_pending = 0;
volatile uint8 car_key1_level = CAR_KEY_RELEASE_LEVEL;
volatile uint8 car_key2_level = CAR_KEY_RELEASE_LEVEL;
volatile uint8 car_error_code = CAR_ERROR_NONE;

volatile int32 car_left_rear_encoder_count = 0;
volatile int32 car_right_rear_encoder_count = 0;
volatile int16 car_left_rear_encoder_delta = 0;
volatile int16 car_right_rear_encoder_delta = 0;
volatile int32 car_steer_encoder_count = 0;
volatile int16 car_steer_encoder_raw_delta = 0;
volatile uint32 car_steer_encoder_reject_count = 0;
volatile int16 car_steer_abs_zero_raw = 0;
volatile int16 car_steer_abs_raw = 0;
volatile float car_steer_angle_raw_deg = 0.0f;
volatile float car_steer_angle_deg = 0.0f;
volatile float car_yaw_deg = 0.0f;
volatile float car_yaw_unwrapped_deg = 0.0f;
volatile float car_speed_mps = 0.0f;
volatile float car_delta_s_m = 0.0f;
volatile float car_gyro_z_dps = 0.0f;
volatile float car_gyro_z_bias = 0.0f;
volatile float car_heading_error_deg = 0.0f;
volatile float car_replay_remaining_m = 0.0f;
volatile float car_target_speed_mps = 0.0f;
volatile float car_target_steer_deg = 0.0f;
volatile int16 car_target_steer_raw = CAR_STEER_ABS_CENTER_RAW;
volatile float car_replay_curvature = 0.0f;
volatile int16 car_rear_pwm_output = 0;
volatile int16 car_steer_pwm_output = 0;
volatile int8 car_steer_stall_block_sign = 0;
volatile int8 car_drive_direction = 0;
volatile uint8 car_remote_active = 0;
volatile uint32 car_remote_rx_count = 0;
volatile uint32 car_remote_ok_count = 0;
volatile int16 car_remote_rear_pwm = 0;
volatile int16 car_remote_steer_target = 0;
volatile uint8 car_wifi_module_ready = 0;
volatile uint8 car_wifi_link_ready = 0;
volatile uint8 car_wifi_stage = 0;
volatile uint8 car_wifi_error = 0;

static car_pid_struct steer_pid = {
    CAR_STEER_POS_KP, CAR_STEER_POS_KI, CAR_STEER_POS_KD,
    CAR_STEER_INTEGRAL_LIMIT, CAR_STEER_PWM_LIMIT, 0.0f, 0.0f, 0.0f
};

static volatile car_vision_command_struct car_vision_command = {0.0f, 0.0f, 0, 0};
static volatile uint32 car_vision_command_seq = 0;
static volatile uint32 car_vision_command_ms = 0;
static uint32 state_enter_ms = 0;
static uint32 last_remote_ms = 0;
static uint32 last_telem_ms = 0;
static uint32 last_wifi_hello_ms = 0;
static uint32 next_wifi_retry_ms = 0;
static uint8 remote_wifi_ready = 0;
static uint8 remote_wifi_ap_ready = 0;
static uint8 remote_wifi_module_ready = 0;
static uint8 remote_wifi_init_busy = 0;
static volatile uint8 remote_pending_flags = 0;
static float steer_target_deg_smooth = 0.0f;
static float steer_prev_angle_deg = 0.0f;
static float steer_angle_rate_dps = 0.0f;
static int16 steer_prev_abs_raw = CAR_STEER_ABS_CENTER_RAW;
static float steer_raw_rate_cps = 0.0f;
static float steer_stall_reference_raw = (float)CAR_STEER_ABS_CENTER_RAW;
static uint16 steer_stall_ticks = 0U;
static int8 steer_stall_command_sign = 0;
static int8 steer_last_applied_sign = 0;
static float rear_pwm_smooth = 0.0f;
static uint8 imu_runtime_fail_count = 0;
static uint16 gyro_stationary_count = 0;
static float gyro_filter_stage1_dps = 0.0f;
static float gyro_filter_stage2_dps = 0.0f;
static uint8 steer_abs_ready = 0;
static int16 steer_abs_zero_raw = 0;
static int16 steer_abs_raw = 0;
static uint8 ready_center_active = 0U;

static float car_absf(float v)
{
    return (v >= 0.0f) ? v : -v;
}

static float car_limitf(float v, float min_v, float max_v)
{
    if(v < min_v) return min_v;
    if(v > max_v) return max_v;
    return v;
}

static int16 car_limiti16(int32 v, int32 min_v, int32 max_v)
{
    if(v < min_v) return (int16)min_v;
    if(v > max_v) return (int16)max_v;
    return (int16)v;
}

static float car_wrap_deg(float a)
{
    while(a > 180.0f) a -= 360.0f;
    while(a < -180.0f) a += 360.0f;
    return a;
}

static void car_enter_state(car_state_enum next)
{
    car_state = next;
    state_enter_ms = car_elapsed_ms;
}

float car_pid_calc(car_pid_struct *pid, float target, float current)
{
    float p;
    float d;
    float output;

    pid->error = target - current;
    pid->integral += pid->error * CAR_CONTROL_PERIOD_S;
    pid->integral = car_limitf(pid->integral, -pid->integral_limit, pid->integral_limit);

    p = pid->kp * pid->error;
    d = pid->kd * (pid->error - pid->last_error) / CAR_CONTROL_PERIOD_S;
    output = p + pid->ki * pid->integral + d;
    pid->last_error = pid->error;

    return car_limitf(output, -pid->output_limit, pid->output_limit);
}

static void car_pid_reset(car_pid_struct *pid)
{
    pid->error = 0.0f;
    pid->last_error = 0.0f;
    pid->integral = 0.0f;
}

static void car_set_signed_motor(gpio_pin_enum dir_pin, pwm_channel_enum pwm_ch,
        gpio_level_enum forward_level, gpio_level_enum reverse_level, int16 duty, float sign)
{
    int32 signed_duty = (int32)((float)duty * sign);
    uint32 abs_duty;

    if(signed_duty >= 0)
    {
        gpio_set_level(dir_pin, forward_level);
        abs_duty = (uint32)signed_duty;
    }
    else
    {
        gpio_set_level(dir_pin, reverse_level);
        abs_duty = (uint32)(-signed_duty);
    }
    pwm_set_duty(pwm_ch, abs_duty);
}

static void car_set_rear_pwm_pair(int16 left_duty, int16 right_duty)
{
    left_duty = car_limiti16(left_duty, -CAR_REAR_DUTY_LIMIT, CAR_REAR_DUTY_LIMIT);
    right_duty = car_limiti16(right_duty, -CAR_REAR_DUTY_LIMIT, CAR_REAR_DUTY_LIMIT);
    car_rear_pwm_output = (int16)(((int32)left_duty + (int32)right_duty) / 2);
    car_set_signed_motor(CAR_LEFT_MOTOR_DIR_PIN, CAR_LEFT_MOTOR_PWM_CH,
            CAR_MOTOR_FORWARD_LEVEL, CAR_MOTOR_REVERSE_LEVEL,
            left_duty, CAR_LEFT_MOTOR_DUTY_SIGN);
    car_set_signed_motor(CAR_RIGHT_MOTOR_DIR_PIN, CAR_RIGHT_MOTOR_PWM_CH,
            CAR_MOTOR_FORWARD_LEVEL, CAR_MOTOR_REVERSE_LEVEL,
            right_duty, CAR_RIGHT_MOTOR_DUTY_SIGN);
}

static void car_set_rear_pwm(int16 duty)
{
    car_set_rear_pwm_pair(duty, duty);
}

static void car_set_steer_pwm(int16 duty)
{
    int8 requested_sign;

    duty = car_limiti16(duty, -CAR_STEER_PWM_LIMIT, CAR_STEER_PWM_LIMIT);
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    if(steer_abs_ready)
    {
        if(((car_steer_abs_raw >= CAR_STEER_ABS_LEFT_SOFT_RAW) && (duty > 0))
                || ((car_steer_abs_raw <= CAR_STEER_ABS_RIGHT_SOFT_RAW) && (duty < 0)))
        {
            duty = 0;
        }
    }
#endif
    if((duty > 0) && (duty < CAR_STEER_MOVE_MIN_DUTY)) duty = CAR_STEER_MOVE_MIN_DUTY;
    if((duty < 0) && (duty > -CAR_STEER_MOVE_MIN_DUTY)) duty = -CAR_STEER_MOVE_MIN_DUTY;

    requested_sign = (duty > 0) ? 1 : ((duty < 0) ? -1 : 0);
    if(requested_sign == 0)
    {
        car_steer_pwm_output = 0;
        pwm_set_duty(CAR_STEER_MOTOR_PWM_CH, 0U);
        steer_last_applied_sign = 0;
        return;
    }

    if((steer_last_applied_sign != 0) && (requested_sign != steer_last_applied_sign))
    {
        car_steer_pwm_output = 0;
        pwm_set_duty(CAR_STEER_MOTOR_PWM_CH, 0U);
        if(((float)requested_sign * CAR_STEER_MOTOR_DUTY_SIGN) >= 0.0f)
        {
            gpio_set_level(CAR_STEER_MOTOR_DIR_PIN, CAR_STEER_FORWARD_LEVEL);
        }
        else
        {
            gpio_set_level(CAR_STEER_MOTOR_DIR_PIN, CAR_STEER_REVERSE_LEVEL);
        }
        steer_last_applied_sign = 0;
        return;
    }

    car_steer_pwm_output = duty;
    car_set_signed_motor(CAR_STEER_MOTOR_DIR_PIN, CAR_STEER_MOTOR_PWM_CH,
            CAR_STEER_FORWARD_LEVEL, CAR_STEER_REVERSE_LEVEL, duty, CAR_STEER_MOTOR_DUTY_SIGN);
    steer_last_applied_sign = requested_sign;
}

void car_all_motor_stop(void)
{
    car_drive_direction = 0;
    car_target_speed_mps = 0.0f;
    car_target_steer_deg = 0.0f;
    car_target_steer_raw = CAR_STEER_ABS_CENTER_RAW;
    rear_pwm_smooth = 0.0f;
    car_set_rear_pwm(0);
    car_set_steer_pwm(0);
}

static int16 car_ramp_rear_pwm_value(int16 target)
{
    float diff = (float)target - rear_pwm_smooth;

    if(diff > CAR_REAR_PWM_SLEW_STEP) diff = CAR_REAR_PWM_SLEW_STEP;
    if(diff < -CAR_REAR_PWM_SLEW_STEP) diff = -CAR_REAR_PWM_SLEW_STEP;
    rear_pwm_smooth += diff;
    return (int16)rear_pwm_smooth;
}

static void car_ramp_rear_pwm(int16 target)
{
    car_set_rear_pwm(car_ramp_rear_pwm_value(target));
}

static void car_ramp_rear_pwm_turn(int16 target, float steer_deg)
{
    int16 base_duty = car_ramp_rear_pwm_value(target);
    int16 left_duty = base_duty;
    int16 right_duty = base_duty;

    if(car_absf(steer_deg) >= CAR_REAR_DIFF_TRIGGER_DEG)
    {
        if(steer_deg > 0.0f)
        {
            left_duty = (int16)((float)base_duty * CAR_REAR_DIFF_INNER_SCALE);
            right_duty = (int16)((float)base_duty * CAR_REAR_DIFF_OUTER_SCALE);
        }
        else
        {
            left_duty = (int16)((float)base_duty * CAR_REAR_DIFF_OUTER_SCALE);
            right_duty = (int16)((float)base_duty * CAR_REAR_DIFF_INNER_SCALE);
        }
    }
    car_set_rear_pwm_pair(left_duty, right_duty);
}

static float car_steer_count_to_angle(float count)
{
    float angle = (count / CAR_STEER_ENC_MAX_ABS_COUNT)
            * CAR_STEER_MAX_ANGLE_DEG * CAR_STEER_ANGLE_SIGN;
    return car_limitf(angle, -CAR_STEER_MAX_ANGLE_DEG, CAR_STEER_MAX_ANGLE_DEG);
}

static float car_steer_abs_count_to_angle(float count)
{
    float angle;

    if(count >= 0.0f)
    {
        angle = (count / CAR_STEER_ABS_LEFT_DELTA_RAW) * CAR_STEER_MAX_ANGLE_DEG;
    }
    else
    {
        angle = (count / CAR_STEER_ABS_RIGHT_DELTA_RAW) * CAR_STEER_MAX_ANGLE_DEG;
    }
    angle *= CAR_STEER_ANGLE_SIGN;
    return car_limitf(angle, -CAR_STEER_MAX_ANGLE_DEG, CAR_STEER_MAX_ANGLE_DEG);
}

static float car_steer_angle_to_count(float angle_deg)
{
    return (angle_deg / CAR_STEER_MAX_ANGLE_DEG)
            * CAR_STEER_ENC_MAX_ABS_COUNT * CAR_STEER_ANGLE_SIGN;
}

static float car_steer_command_to_abs_raw(float command_deg)
{
    float raw;

    command_deg = car_limitf(command_deg,
            -CAR_STEER_MAX_ANGLE_DEG, CAR_STEER_MAX_ANGLE_DEG);
    if(command_deg >= 0.0f)
    {
        raw = (float)CAR_STEER_ABS_CENTER_RAW
                + command_deg / CAR_STEER_MAX_ANGLE_DEG
                * (float)(CAR_STEER_ABS_LEFT_LIMIT_RAW - CAR_STEER_ABS_CENTER_RAW);
    }
    else
    {
        raw = (float)CAR_STEER_ABS_CENTER_RAW
                + command_deg / CAR_STEER_MAX_ANGLE_DEG
                * (float)(CAR_STEER_ABS_CENTER_RAW - CAR_STEER_ABS_RIGHT_LIMIT_RAW);
    }
    return car_limitf(raw,
            (float)CAR_STEER_ABS_RIGHT_SOFT_RAW,
            (float)CAR_STEER_ABS_LEFT_SOFT_RAW);
}

static int16 car_steer_position_output_raw(float error_raw, float rate_raw_cps)
{
    float abs_error = car_absf(error_raw);
    float drive;
    float duty;
    float duty_limit;

    if(abs_error <= CAR_STEER_RAW_POSITION_TOL)
    {
        return 0;
    }

    drive = CAR_STEER_RAW_KP * error_raw - CAR_STEER_RAW_KD * rate_raw_cps;
    if((drive * error_raw) <= 0.0f)
    {
        return 0;
    }

    duty = car_absf(drive);
    if(abs_error <= CAR_STEER_RAW_ERROR_NEAR)
    {
        duty_limit = (float)CAR_STEER_PWM_NEAR_DUTY;
    }
    else if(abs_error <= CAR_STEER_RAW_ERROR_FAR)
    {
        duty_limit = (float)CAR_STEER_PWM_MID_DUTY;
    }
    else
    {
        duty_limit = (float)CAR_STEER_PWM_FAR_DUTY;
    }

    duty = car_limitf(duty, (float)CAR_STEER_PWM_START_DUTY, duty_limit);

    if(error_raw < 0.0f)
    {
        duty = -duty;
    }
    return car_limiti16((int32)duty, -CAR_STEER_PWM_LIMIT, CAR_STEER_PWM_LIMIT);
}

static int16 car_steer_stall_guard(int16 output)
{
    int8 command_sign = (output > 0) ? 1 : ((output < 0) ? -1 : 0);

    if(command_sign == 0)
    {
        steer_stall_ticks = 0U;
        steer_stall_command_sign = 0;
        steer_stall_reference_raw = (float)car_steer_abs_raw;
        return 0;
    }

    if(car_steer_stall_block_sign != 0)
    {
        if(command_sign == car_steer_stall_block_sign)
        {
            return 0;
        }
        car_steer_stall_block_sign = 0;
        steer_stall_ticks = 0U;
        steer_stall_command_sign = command_sign;
        steer_stall_reference_raw = (float)car_steer_abs_raw;
    }

    if(command_sign != steer_stall_command_sign)
    {
        steer_stall_command_sign = command_sign;
        steer_stall_ticks = 0U;
        steer_stall_reference_raw = (float)car_steer_abs_raw;
    }
    else if(car_absf((float)car_steer_abs_raw - steer_stall_reference_raw)
            >= CAR_STEER_STALL_MOVE_RAW)
    {
        steer_stall_ticks = 0U;
        steer_stall_reference_raw = (float)car_steer_abs_raw;
    }
    else if(car_absf((float)output) >= (float)CAR_STEER_STALL_MIN_DUTY)
    {
        if(steer_stall_ticks < CAR_STEER_STALL_TICKS)
        {
            steer_stall_ticks++;
        }
        if(steer_stall_ticks >= CAR_STEER_STALL_TICKS)
        {
            car_steer_stall_block_sign = command_sign;
            return 0;
        }
    }

    return output;
}

static void car_steer_decoder_reset(void)
{
    car_steer_encoder_count = 0;
    car_steer_encoder_raw_delta = 0;
    car_steer_encoder_reject_count = 0;
    car_steer_angle_raw_deg = 0.0f;
    car_steer_angle_deg = 0.0f;
    steer_prev_angle_deg = 0.0f;
    steer_angle_rate_dps = 0.0f;
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps = 0.0f;
    steer_stall_reference_raw = (float)car_steer_abs_raw;
    steer_stall_ticks = 0U;
    steer_stall_command_sign = 0;
    steer_last_applied_sign = 0;
    car_steer_stall_block_sign = 0;
}

static int16 car_steer_abs_delta_raw(int16 raw, int16 zero)
{
    int16 delta = raw - zero;
    const int16 half_period = (int16)(CAR_STEER_ABS_RAW_PERIOD / 2);

    if(delta > half_period) delta -= CAR_STEER_ABS_RAW_PERIOD;
    if(delta < -half_period) delta += CAR_STEER_ABS_RAW_PERIOD;
    return delta;
}

static float car_steer_measured_center_deg(void)
{
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    int16 center_delta = car_steer_abs_delta_raw(car_steer_abs_raw,
            CAR_STEER_ABS_CENTER_RAW);
    return car_steer_abs_count_to_angle((float)center_delta);
#else
    return car_steer_angle_deg;
#endif
}

static uint8 car_steer_abs_init(void)
{
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    uint8 i;
    int16 first_raw;
    int16 raw;
    int32 zero_sum;
    int32 zero_avg;

    if(0 != absolute_encoder_init())
    {
        steer_abs_ready = 0U;
        car_error_code = CAR_ERROR_ENCODER;
        return 1U;
    }

    first_raw = absolute_encoder_get_location();
    zero_sum = first_raw;
    for(i = 1U; i < CAR_STEER_ABS_ZERO_SAMPLE_COUNT; i++)
    {
        raw = absolute_encoder_get_location();
        zero_sum += first_raw + car_steer_abs_delta_raw(raw, first_raw);
    }

    zero_avg = zero_sum / CAR_STEER_ABS_ZERO_SAMPLE_COUNT;
    while(zero_avg < 0) zero_avg += CAR_STEER_ABS_RAW_PERIOD;
    while(zero_avg >= CAR_STEER_ABS_RAW_PERIOD) zero_avg -= CAR_STEER_ABS_RAW_PERIOD;

    steer_abs_zero_raw = (int16)zero_avg;
    steer_abs_raw = steer_abs_zero_raw;
    car_steer_abs_zero_raw = steer_abs_zero_raw;
    car_steer_abs_raw = steer_abs_raw;
    steer_abs_ready = 1U;
    car_steer_decoder_reset();
#endif
    return 0U;
}

static void car_drive_steer_angle(float target_deg)
{
    int16 output;
    float target_raw;
    float measured_rate_raw_cps;
    float delta;
    float error_raw;

    target_deg = car_limitf(target_deg, -CAR_STEER_TARGET_LIMIT_DEG, CAR_STEER_TARGET_LIMIT_DEG);
    if(car_absf(target_deg) >= (CAR_STEER_TARGET_LIMIT_DEG - 0.1f))
    {
        steer_target_deg_smooth = target_deg;
    }
    else
    {
        delta = target_deg - steer_target_deg_smooth;
        if(delta > CAR_STEER_TARGET_SLEW_DEG) delta = CAR_STEER_TARGET_SLEW_DEG;
        if(delta < -CAR_STEER_TARGET_SLEW_DEG) delta = -CAR_STEER_TARGET_SLEW_DEG;
        steer_target_deg_smooth += delta;
    }

    car_target_steer_deg = steer_target_deg_smooth;
    target_raw = car_steer_command_to_abs_raw(steer_target_deg_smooth);
    car_target_steer_raw = (int16)target_raw;
    error_raw = target_raw - (float)car_steer_abs_raw;
    measured_rate_raw_cps = (float)car_steer_abs_delta_raw(car_steer_abs_raw,
            steer_prev_abs_raw) / CAR_CONTROL_PERIOD_S;
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps += (measured_rate_raw_cps - steer_raw_rate_cps)
            * CAR_STEER_RATE_FILTER_ALPHA;
    output = car_steer_position_output_raw(error_raw, steer_raw_rate_cps);
    steer_pid.error = error_raw;
    steer_pid.last_error = error_raw;
    steer_pid.integral = 0.0f;

    // A soft limit blocks only motion farther into the stop; reversing remains available.
    if(((car_steer_abs_raw >= CAR_STEER_ABS_LEFT_SOFT_RAW) && (output > 0))
            || ((car_steer_abs_raw <= CAR_STEER_ABS_RIGHT_SOFT_RAW) && (output < 0)))
    {
        output = 0;
        steer_pid.integral = 0.0f;
    }
    output = car_steer_stall_guard(output);
    car_set_steer_pwm(output);
}

static void car_drive_steer_count(float target_count)
{
    float target_deg;

    target_count = car_limitf(target_count, -CAR_REMOTE_STEER_LIMIT_CNT, CAR_REMOTE_STEER_LIMIT_CNT);
    target_deg = car_steer_count_to_angle(target_count);
    car_drive_steer_angle(target_deg);
}

static void car_encoder_init_one(encoder_index_enum encoder_n,
        encoder_channel1_enum ch1, encoder_channel2_enum ch2, uint8 mode)
{
    if(CAR_ENCODER_MODE_DIR == mode)
    {
        encoder_dir_init(encoder_n, ch1, ch2);
    }
    else
    {
        encoder_quad_init(encoder_n, ch1, ch2);
    }
    encoder_clear_count(encoder_n);
}

static void car_hardware_init(void)
{
    gpio_init(CAR_LEFT_MOTOR_DIR_PIN, GPO, CAR_MOTOR_FORWARD_LEVEL, GPO_PUSH_PULL);
    gpio_init(CAR_RIGHT_MOTOR_DIR_PIN, GPO, CAR_MOTOR_FORWARD_LEVEL, GPO_PUSH_PULL);
    gpio_init(CAR_STEER_MOTOR_DIR_PIN, GPO, CAR_STEER_FORWARD_LEVEL, GPO_PUSH_PULL);
    pwm_init(CAR_LEFT_MOTOR_PWM_CH, CAR_PWM_FREQ_HZ, 0);
    pwm_init(CAR_RIGHT_MOTOR_PWM_CH, CAR_PWM_FREQ_HZ, 0);
    pwm_init(CAR_STEER_MOTOR_PWM_CH, CAR_PWM_FREQ_HZ, 0);

    car_encoder_init_one(CAR_LEFT_REAR_ENCODER,
            CAR_LEFT_REAR_ENCODER_CH1_PIN, CAR_LEFT_REAR_ENCODER_CH2_PIN,
            CAR_LEFT_REAR_ENCODER_MODE);
    car_encoder_init_one(CAR_RIGHT_REAR_ENCODER,
            CAR_RIGHT_REAR_ENCODER_CH1_PIN, CAR_RIGHT_REAR_ENCODER_CH2_PIN,
            CAR_RIGHT_REAR_ENCODER_MODE);
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    (void)car_steer_abs_init();
#else
    car_encoder_init_one(CAR_STEER_ENCODER,
            CAR_STEER_ENCODER_CH1_PIN, CAR_STEER_ENCODER_CH2_PIN,
            CAR_STEER_ENCODER_MODE);
#endif

#if CAR_USE_BOARD_KEYS
    gpio_init(CAR_GUIDE_KEY_PIN, GPI, CAR_KEY_RELEASE_LEVEL, GPI_PULL_UP);
    gpio_init(CAR_RESET_KEY_PIN, GPI, CAR_KEY_RELEASE_LEVEL, GPI_PULL_UP);
#endif
}

static void car_reset_pose(void)
{
    car_yaw_deg = 0.0f;
    car_yaw_unwrapped_deg = 0.0f;
    car_speed_mps = 0.0f;
    car_delta_s_m = 0.0f;
    car_gyro_z_dps = 0.0f;
    car_heading_error_deg = 0.0f;
    car_replay_remaining_m = 0.0f;
    car_replay_curvature = 0.0f;
    gyro_stationary_count = 0;
    gyro_filter_stage1_dps = 0.0f;
    gyro_filter_stage2_dps = 0.0f;
}

static uint8 car_calibrate_gyro_bias(void)
{
    uint32 i;
    uint32 valid_count = 0;
    float sum = 0.0f;

    system_delay_ms(CAR_GYRO_STARTUP_SETTLE_MS);
    gyro_filter_stage1_dps = 0.0f;
    gyro_filter_stage2_dps = 0.0f;
    car_gyro_z_dps = 0.0f;

    for(i = 0; i < CAR_GYRO_BIAS_SAMPLE_COUNT; i++)
    {
        if(0 == car_imu_update())
        {
            sum += car_imu_yaw_rate_dps();
            valid_count++;
        }
        system_delay_ms(CAR_GYRO_BIAS_SAMPLE_DELAY_MS);
    }

    if(valid_count < CAR_IMU_BIAS_MIN_VALID_COUNT)
    {
        return 1;
    }

    car_gyro_z_bias = sum / (float)valid_count;
    gyro_stationary_count = 0;
    gyro_filter_stage1_dps = 0.0f;
    gyro_filter_stage2_dps = 0.0f;
    car_gyro_z_dps = 0.0f;
    car_yaw_deg = 0.0f;
    car_yaw_unwrapped_deg = 0.0f;
    return 0;
}

static void car_sensor_update(void)
{
    int16 left_delta;
    int16 right_delta;
    int16 steer_delta;
    float left_meter_per_count;
    float right_meter_per_count;
    float ds_left;
    float ds_right;
    float raw_speed_mps;
    float yaw_rate;
    float raw_yaw_rate;
    float gyro_bias_error;

    left_delta = encoder_get_count(CAR_LEFT_REAR_ENCODER);
    right_delta = encoder_get_count(CAR_RIGHT_REAR_ENCODER);
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    steer_delta = 0;
#else
    steer_delta = encoder_get_count(CAR_STEER_ENCODER);
#endif
    encoder_clear_count(CAR_LEFT_REAR_ENCODER);
    encoder_clear_count(CAR_RIGHT_REAR_ENCODER);
#if CAR_STEER_SENSOR_TYPE != CAR_STEER_SENSOR_ABSOLUTE_SPI
    encoder_clear_count(CAR_STEER_ENCODER);
#endif

    left_delta = (int16)((float)left_delta * CAR_LEFT_REAR_ENCODER_SIGN);
    right_delta = (int16)((float)right_delta * CAR_RIGHT_REAR_ENCODER_SIGN);
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    if(steer_abs_ready)
    {
        steer_abs_raw = absolute_encoder_get_location();
        car_steer_abs_raw = steer_abs_raw;
        car_steer_abs_zero_raw = steer_abs_zero_raw;
        steer_delta = car_steer_abs_delta_raw(steer_abs_raw, steer_abs_zero_raw);
    }
#else
    steer_delta = (int16)((float)steer_delta * CAR_STEER_ENCODER_SIGN);
#endif
    car_steer_encoder_raw_delta = steer_delta;
    car_left_rear_encoder_delta = left_delta;
    car_right_rear_encoder_delta = right_delta;

    car_left_rear_encoder_count += left_delta;
    car_right_rear_encoder_count += right_delta;
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    car_steer_encoder_count = steer_delta;
#else
    car_steer_encoder_count += steer_delta;
#endif
#if CAR_STEER_SENSOR_TYPE == CAR_STEER_SENSOR_ABSOLUTE_SPI
    car_steer_angle_raw_deg = car_steer_abs_count_to_angle((float)car_steer_encoder_count);
#else
    car_steer_angle_raw_deg = car_steer_count_to_angle((float)car_steer_encoder_count);
#endif
    car_steer_angle_deg = car_steer_angle_raw_deg;

    left_meter_per_count = (3.1415926536f * CAR_REAR_WHEEL_DIAMETER_M)
            / CAR_LEFT_REAR_COUNT_PER_REV;
    right_meter_per_count = (3.1415926536f * CAR_REAR_WHEEL_DIAMETER_M)
            / CAR_RIGHT_REAR_COUNT_PER_REV;
    ds_left = (float)left_delta * left_meter_per_count;
    ds_right = (float)right_delta * right_meter_per_count;
#if CAR_REAR_ENCODER_SOURCE == CAR_REAR_ENCODER_SOURCE_LEFT
    car_delta_s_m = ds_left;
#elif CAR_REAR_ENCODER_SOURCE == CAR_REAR_ENCODER_SOURCE_RIGHT
    car_delta_s_m = ds_right;
#else
    car_delta_s_m = 0.5f * (ds_left + ds_right);
#endif
    raw_speed_mps = car_delta_s_m / CAR_CONTROL_PERIOD_S;
    car_speed_mps += (raw_speed_mps - car_speed_mps) * CAR_SPEED_LOWPASS_ALPHA;

    if(0 == car_imu_update())
    {
        imu_runtime_fail_count = 0;
        raw_yaw_rate = car_imu_yaw_rate_dps();
        gyro_bias_error = raw_yaw_rate - car_gyro_z_bias;

        if((car_absf((float)left_delta) <= CAR_GYRO_STATIONARY_ENC_MAX)
                && (car_absf((float)right_delta) <= CAR_GYRO_STATIONARY_ENC_MAX)
                && (0 == car_rear_pwm_output)
                && (CAR_STATE_WAIT == car_state)
                && !car_path_is_ready()
                && (car_absf(gyro_bias_error)
                    <= CAR_GYRO_BIAS_TRACK_MAX_ERROR_DPS))
        {
            if(gyro_stationary_count < CAR_GYRO_STATIONARY_COUNT) gyro_stationary_count++;
            else
            {
                car_gyro_z_bias += gyro_bias_error * CAR_GYRO_BIAS_TRACK_ALPHA;
                gyro_bias_error = raw_yaw_rate - car_gyro_z_bias;
            }
        }
        else
        {
            gyro_stationary_count = 0;
        }

        yaw_rate = gyro_bias_error * CAR_GYRO_SCALE;
        if(car_absf(yaw_rate) < CAR_GYRO_DEADBAND_DPS) yaw_rate = 0.0f;
        gyro_filter_stage1_dps += (yaw_rate - gyro_filter_stage1_dps) * CAR_GYRO_LOWPASS_ALPHA;
        gyro_filter_stage2_dps += (gyro_filter_stage1_dps - gyro_filter_stage2_dps) * CAR_GYRO_LOWPASS_ALPHA;
        car_gyro_z_dps = gyro_filter_stage2_dps;
    }
    else if(imu_runtime_fail_count < 255)
    {
        imu_runtime_fail_count++;
    }

    if(imu_runtime_fail_count >= CAR_IMU_RUNTIME_FAIL_CONFIRM)
    {
        car_error_code = CAR_ERROR_IMU_LOST;
        car_enter_state(CAR_STATE_ERROR);
        car_all_motor_stop();
    }

    car_yaw_unwrapped_deg += car_gyro_z_dps * CAR_CONTROL_PERIOD_S;
    car_yaw_deg = car_wrap_deg(car_yaw_unwrapped_deg);
}

#if CAR_STEER_BOOT_SELF_TEST_ENABLE
static void car_steer_self_test_step(float target_deg, uint32 hold_ms)
{
    uint32 move_ms = 0U;
    uint32 stable_ms = 0U;

    car_pid_reset(&steer_pid);
    while(move_ms < CAR_STEER_SELF_TEST_TIMEOUT_MS)
    {
        car_set_rear_pwm(0);
        car_sensor_update();
        car_drive_steer_angle(target_deg);

        if(car_absf(car_steer_angle_deg - car_target_steer_deg)
                <= CAR_STEER_POSITION_TOL_DEG)
        {
            stable_ms += CAR_CONTROL_PERIOD_MS;
            if(stable_ms >= CAR_STEER_SELF_TEST_STABLE_MS)
            {
                break;
            }
        }
        else
        {
            stable_ms = 0U;
        }

        system_delay_ms(CAR_CONTROL_PERIOD_MS);
        move_ms += CAR_CONTROL_PERIOD_MS;
    }

    car_set_rear_pwm(0);
    car_set_steer_pwm(0);
    if(hold_ms > 0U)
    {
        system_delay_ms(hold_ms);
    }
}

static void car_steer_boot_self_test(void)
{
    car_set_rear_pwm(0);
    car_pid_reset(&steer_pid);
    steer_target_deg_smooth = car_steer_angle_deg;
    car_target_steer_deg = car_steer_angle_deg;

    car_steer_self_test_step(-CAR_STEER_SELF_TEST_TARGET_DEG, CAR_STEER_SELF_TEST_HOLD_MS);
    car_steer_self_test_step(0.0f, CAR_STEER_SELF_TEST_HOLD_MS);
    car_steer_self_test_step(CAR_STEER_SELF_TEST_TARGET_DEG, CAR_STEER_SELF_TEST_HOLD_MS);
    car_steer_self_test_step(0.0f, CAR_STEER_SELF_TEST_HOLD_MS);

    car_set_rear_pwm(0);
    car_set_steer_pwm(0);
    car_pid_reset(&steer_pid);
    steer_target_deg_smooth = 0.0f;
    car_target_steer_deg = 0.0f;
}
#endif

static void car_start_record(void)
{
    car_all_motor_stop();
    car_reset_pose();
    car_path_clear();
    car_pid_reset(&steer_pid);
    car_follow_reset();
    steer_target_deg_smooth = 0.0f;
    steer_prev_angle_deg = car_steer_angle_deg;
    steer_angle_rate_dps = 0.0f;
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps = 0.0f;
    steer_stall_reference_raw = (float)car_steer_abs_raw;
    steer_stall_ticks = 0U;
    steer_stall_command_sign = 0;
    car_steer_stall_block_sign = 0;
    rear_pwm_smooth = 0.0f;
    car_error_code = CAR_ERROR_NONE;
    car_drive_direction = 0;
    ready_center_active = 0U;
    car_enter_state(CAR_STATE_FOLLOW_RECORD);
    car_path_record_begin(car_yaw_unwrapped_deg,
            car_steer_measured_center_deg(), car_elapsed_ms);
}

static void car_finish_record(void)
{
    car_path_record_finish(car_yaw_unwrapped_deg,
            car_steer_measured_center_deg(), car_elapsed_ms);
    car_all_motor_stop();

    if(!car_path_is_ready())
    {
        car_error_code = car_path_full ? CAR_ERROR_RECORD_FULL : CAR_ERROR_PATH_SHORT;
        car_enter_state(car_path_full ? CAR_STATE_RECORD_FULL : CAR_STATE_ERROR);
        return;
    }

#if CAR_PATH_SAVE_TO_FLASH
    if(car_path_flash_save() == 0U)
    {
        car_error_code = CAR_ERROR_FLASH;
        car_enter_state(CAR_STATE_ERROR);
        return;
    }
#else
    car_path_saved_valid = 0U;
#endif

    car_pid_reset(&steer_pid);
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps = 0.0f;
    steer_stall_reference_raw = (float)car_steer_abs_raw;
    steer_stall_ticks = 0U;
    steer_stall_command_sign = 0;
    car_steer_stall_block_sign = 0;
    ready_center_active = 1U;
    car_enter_state(CAR_STATE_WAIT);
}

static void car_update_ready_center(void)
{
    int32 center_error_raw = (int32)CAR_STEER_ABS_CENTER_RAW
            - (int32)car_steer_abs_raw;

    car_set_rear_pwm(0);
    car_drive_direction = 0;
    if((center_error_raw >= -CAR_STEER_READY_CENTER_TOL_RAW)
            && (center_error_raw <= CAR_STEER_READY_CENTER_TOL_RAW))
    {
        ready_center_active = 0U;
        steer_target_deg_smooth = 0.0f;
        car_target_steer_deg = 0.0f;
        car_target_steer_raw = CAR_STEER_ABS_CENTER_RAW;
        car_set_steer_pwm(0);
        return;
    }

    car_drive_steer_angle(0.0f);
}

static void car_start_return(void)
{
    if(!car_path_is_ready())
    {
        car_error_code = CAR_ERROR_PATH_SHORT;
        car_enter_state(CAR_STATE_ERROR);
        return;
    }
    car_replay_reset();
    car_drive_direction = -1;
    car_pid_reset(&steer_pid);
    steer_target_deg_smooth = 0.0f;
    steer_prev_angle_deg = car_steer_angle_deg;
    steer_angle_rate_dps = 0.0f;
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps = 0.0f;
    steer_stall_reference_raw = (float)car_steer_abs_raw;
    steer_stall_ticks = 0U;
    steer_stall_command_sign = 0;
    car_steer_stall_block_sign = 0;
    rear_pwm_smooth = 0.0f;
    car_error_code = CAR_ERROR_NONE;
    car_enter_state(CAR_STATE_RETURN);
}

static void car_finish_run(void)
{
    car_all_motor_stop();
    car_pid_reset(&steer_pid);
    steer_prev_abs_raw = car_steer_abs_raw;
    steer_raw_rate_cps = 0.0f;
    steer_stall_reference_raw = (float)car_steer_abs_raw;
    steer_stall_ticks = 0U;
    steer_stall_command_sign = 0;
    car_steer_stall_block_sign = 0;
    ready_center_active = 1U;
    car_enter_state(CAR_STATE_FINISHED);
}

static void car_remote_parse_command(uint8 flags)
{
    remote_pending_flags |= flags;
    __dsync();
}

static void car_remote_apply_pending_commands(void)
{
    uint8 flags = remote_pending_flags;

    if(0U == flags)
    {
        return;
    }
    remote_pending_flags = 0U;
    __dsync();

    if(flags & CAR_REMOTE_FLAG_RESET)
    {
        car_all_motor_stop();
        car_reset_pose();
        car_error_code = CAR_ERROR_NONE;
#if CAR_PATH_SAVE_TO_FLASH
        (void)car_path_flash_load();
#else
        car_path_clear();
#endif
        car_enter_state(CAR_STATE_WAIT);
    }
    if(flags & CAR_REMOTE_FLAG_FINISH)
    {
        if(CAR_STATE_FOLLOW_RECORD == car_state)
        {
            car_finish_record();
        }
        else
        {
            car_finish_run();
        }
    }
    if(flags & CAR_REMOTE_FLAG_READY)
    {
        if(CAR_STATE_FOLLOW_RECORD == car_state)
        {
            car_finish_record();
        }
        else if((CAR_STATE_WAIT == car_state) || (CAR_STATE_FINISHED == car_state)
                || (CAR_STATE_ERROR == car_state) || (CAR_STATE_RECORD_FULL == car_state))
        {
            car_start_record();
        }
    }
    if(flags & CAR_REMOTE_FLAG_GUIDE)
    {
        if((CAR_STATE_PARK_STOP == car_state) || (CAR_STATE_FINISHED == car_state)
                || (CAR_STATE_WAIT == car_state) || (CAR_STATE_ERROR == car_state))
        {
            car_start_return();
        }
    }
}

static void car_remote_parse_byte(uint8 data)
{
    static uint8 state = 0;
    static uint8 buffer[9];
    static uint8 index = 0;
    uint8 payload_sum;
    uint8 full_sum;

    car_remote_rx_count++;
    switch(state)
    {
        case 0:
            if(CAR_REMOTE_FRAME_HEAD1 == data)
            {
                buffer[0] = data;
                index = 1;
                state = 1;
            }
            break;
        case 1:
            if(CAR_REMOTE_FRAME_HEAD2 == data)
            {
                buffer[index++] = data;
                state = 2;
            }
            else
            {
                state = 0;
            }
            break;
        case 2:
            buffer[index++] = data;
            if(index >= sizeof(buffer))
            {
                state = 0;
                if(buffer[8] != CAR_REMOTE_FRAME_TAIL) break;
                payload_sum = (uint8)(buffer[2] + buffer[3] + buffer[4] + buffer[5] + buffer[6]);
                full_sum = (uint8)(buffer[0] + buffer[1] + payload_sum);
                if((payload_sum != buffer[7]) && (full_sum != buffer[7])) break;

                car_remote_rear_pwm = (int16)(((uint16)buffer[2] << 8) | buffer[3]);
                car_remote_steer_target = (int16)(((uint16)buffer[4] << 8) | buffer[5]);
                car_remote_rear_pwm = car_limiti16(car_remote_rear_pwm,
                        -CAR_REMOTE_REAR_PWM_LIMIT, CAR_REMOTE_REAR_PWM_LIMIT);
                car_remote_steer_target = car_limiti16(car_remote_steer_target,
                        -(int32)CAR_REMOTE_STEER_LIMIT_CNT, (int32)CAR_REMOTE_STEER_LIMIT_CNT);
                car_remote_active = (buffer[6] & CAR_REMOTE_FLAG_ACTIVE) ? 1 : 0;
                last_remote_ms = car_elapsed_ms;
                car_remote_ok_count++;
                car_remote_parse_command(buffer[6]);
            }
            break;
        default:
            state = 0;
            break;
    }
}

static uint8 car_text_has_token(const uint8 *buffer, uint32 length, const char *token)
{
    uint32 i;
    uint32 j;
    uint32 token_len = (uint32)strlen(token);

    if(0 == token_len || length < token_len) return 0;
    for(i = 0; i + token_len <= length; i++)
    {
        for(j = 0; j < token_len; j++)
        {
            char c = (char)buffer[i + j];
            if((c >= 'a') && (c <= 'z')) c = (char)(c - 'a' + 'A');
            if(c != token[j]) break;
        }
        if(j == token_len) return 1;
    }
    return 0;
}

static uint8 car_remote_parse_ascii_packet(const uint8 *buffer, uint32 length)
{
    uint8 handled = 0;

    if(0 == length) return 0;
    if(car_text_has_token(buffer, length, "RESET"))
    {
        car_remote_parse_command(CAR_REMOTE_FLAG_RESET);
        handled = 1;
    }
    if(car_text_has_token(buffer, length, "READY") || car_text_has_token(buffer, length, "START")
            || car_text_has_token(buffer, length, "REC"))
    {
        car_remote_parse_command(CAR_REMOTE_FLAG_READY);
        handled = 1;
    }
    if(car_text_has_token(buffer, length, "GUIDE") || car_text_has_token(buffer, length, "RETURN"))
    {
        car_remote_parse_command(CAR_REMOTE_FLAG_GUIDE);
        handled = 1;
    }
    if(car_text_has_token(buffer, length, "FINISH") || car_text_has_token(buffer, length, "STOP"))
    {
        car_remote_parse_command(CAR_REMOTE_FLAG_FINISH);
        handled = 1;
    }
    if(handled)
    {
        last_remote_ms = car_elapsed_ms;
        car_remote_ok_count++;
    }
    return handled;
}

void car_remote_uart_callback(void)
{
#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_UART
    uint8 data;
    while(uart_query_byte(CAR_REMOTE_UART_INDEX, &data))
    {
        car_remote_parse_byte(data);
    }
#endif
}

static void car_remote_wifi_poll(void);
static void car_remote_send_telemetry(void);

static void car_remote_wifi_init(void)
{
#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_WIFI_SPI
    uint8 result;

    car_wifi_module_ready = 0;
    car_wifi_link_ready = 0;
    car_wifi_stage = 3;
    car_wifi_error = 0;
    remote_wifi_module_ready = 0;
    remote_wifi_ap_ready = 0;
    remote_wifi_ready = 0;

    result = wifi_spi_init(NULL, NULL);
    if(0 != result)
    {
        car_wifi_error = 1;
        return;
    }

    remote_wifi_module_ready = 1;
    car_wifi_module_ready = 1;
    car_wifi_stage = 4;

    result = wifi_spi_wifi_connect(CAR_REMOTE_WIFI_SSID, CAR_REMOTE_WIFI_PASSWORD);
    if(0 != result)
    {
        car_wifi_error = 1;
        return;
    }

    remote_wifi_ap_ready = 1;
    car_wifi_stage = 5;
    result = wifi_spi_socket_connect("UDP", CAR_REMOTE_WIFI_PC_IP,
            CAR_REMOTE_WIFI_PC_PORT, CAR_REMOTE_WIFI_LOCAL_PORT);
    if(0 != result)
    {
        car_wifi_error = 2;
        return;
    }

    remote_wifi_ready = 1;
    car_wifi_link_ready = 1;
    car_wifi_stage = 6;
#endif
}

void car_remote_wifi_task(void)
{
#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_WIFI_SPI
    if((0 == remote_wifi_module_ready) && (0 == remote_wifi_init_busy)
            && (car_elapsed_ms >= next_wifi_retry_ms)
            && ((CAR_STATE_WAIT == car_state) || (CAR_STATE_ERROR == car_state)
                || (CAR_STATE_FINISHED == car_state)))
    {
        remote_wifi_init_busy = 1;
        car_remote_wifi_init();
        remote_wifi_init_busy = 0;
        next_wifi_retry_ms = car_elapsed_ms + 5000U;
    }

    if(remote_wifi_module_ready != 0U)
    {
        car_remote_wifi_poll();
    }
    car_remote_send_telemetry();
#endif
}

static void car_remote_wifi_poll(void)
{
#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_WIFI_SPI
    uint8 buffer[128];
    uint32 length;
    uint32 i;

    if(0 == remote_wifi_module_ready)
    {
        return;
    }

    if(0 == remote_wifi_ap_ready)
    {
        if(car_elapsed_ms >= next_wifi_retry_ms)
        {
            next_wifi_retry_ms = car_elapsed_ms + 3000U;
            car_wifi_stage = 4;
            if(0 == wifi_spi_wifi_connect(CAR_REMOTE_WIFI_SSID, CAR_REMOTE_WIFI_PASSWORD))
            {
                remote_wifi_ap_ready = 1;
                car_wifi_error = 0;
                car_wifi_stage = 5;
                next_wifi_retry_ms = car_elapsed_ms;
            }
            else
            {
                car_wifi_error = 1;
            }
        }
        return;
    }

    if(0 == remote_wifi_ready)
    {
        if(car_elapsed_ms >= next_wifi_retry_ms)
        {
            next_wifi_retry_ms = car_elapsed_ms + 1500U;
            car_wifi_stage = 5;
            if(0 == wifi_spi_socket_connect("UDP", CAR_REMOTE_WIFI_PC_IP,
                    CAR_REMOTE_WIFI_PC_PORT, CAR_REMOTE_WIFI_LOCAL_PORT))
            {
                remote_wifi_ready = 1;
                car_wifi_link_ready = 1;
                car_wifi_error = 0;
                car_wifi_stage = 6;
                last_wifi_hello_ms = 0;
            }
            else
            {
                car_wifi_error = 2;
            }
        }
        return;
    }

    length = wifi_spi_read_buffer(buffer, sizeof(buffer));
    if(length)
    {
        if(!car_remote_parse_ascii_packet(buffer, length))
        {
            for(i = 0; i < length; i++)
            {
                car_remote_parse_byte(buffer[i]);
            }
        }
    }

    if((car_elapsed_ms - last_wifi_hello_ms) >= CAR_REMOTE_WIFI_HELLO_MS)
    {
        last_wifi_hello_ms = car_elapsed_ms;
        wifi_spi_send_string("KM13_HELLO\r\n");
        wifi_spi_udp_send_now();
    }
#endif
}

static void car_remote_send_telemetry(void)
{
#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_WIFI_SPI
    char text[256];
    int length;

    if(!remote_wifi_ready) return;
    if((car_elapsed_ms - last_telem_ms) < CAR_REMOTE_TELEM_MS) return;
    last_telem_ms = car_elapsed_ms;

    length = sprintf(text,
            "TEL KM13 t=%lu sta=%u err=%u yaw=%ld gy=%ld v=%ld ds=%ld pts=%lu s=%ld rs=%ld rem=%ld idx=%lu he=%ld cur=%ld st=%ld ed=%ld le=%ld re=%ld rp=%d sp=%d ra=%u ro=%lu\r\n",
            car_elapsed_ms,
            (uint8)car_state,
            car_error_code,
            (int32)(car_yaw_unwrapped_deg * 10.0f),
            (int32)(car_gyro_z_dps * 10.0f),
            (int32)(car_speed_mps * 1000.0f),
            (int32)(car_delta_s_m * 1000000.0f),
            car_path_count,
            (int32)(car_record_s_m * 1000.0f),
            (int32)(car_replay_s_m * 1000.0f),
            (int32)(car_replay_remaining_m * 1000.0f),
            car_replay_index,
            (int32)(car_heading_error_deg * 10.0f),
            (int32)(car_replay_curvature * 1000.0f),
            (int32)(car_target_steer_deg * 10.0f),
            car_steer_encoder_count,
            car_left_rear_encoder_count,
            car_right_rear_encoder_count,
            car_rear_pwm_output,
            car_steer_pwm_output,
            car_remote_active,
            car_remote_ok_count);
    if(length > 0)
    {
        wifi_spi_send_buffer((const uint8 *)text, (uint32)length);
        wifi_spi_udp_send_now();
    }
#endif
}

static void car_update_keys(void)
{
#if CAR_USE_BOARD_KEYS
    static uint16 key1_cooldown = 0;
    static uint16 key2_cooldown = 0;
    static uint8 key1_last = CAR_KEY_RELEASE_LEVEL;
    static uint8 key2_last = CAR_KEY_RELEASE_LEVEL;
    uint8 key1;
    uint8 key2;
    uint8 key1_pressed;
    uint8 key2_pressed;

    key1 = gpio_get_level(CAR_GUIDE_KEY_PIN);
    key2 = gpio_get_level(CAR_RESET_KEY_PIN);
    car_key1_level = key1;
    car_key2_level = key2;

    if(key1_cooldown) key1_cooldown--;
    if(key2_cooldown) key2_cooldown--;

    key1_pressed = (uint8)((key1 == CAR_KEY_ACTIVE_LEVEL)
            && (key1_last == CAR_KEY_RELEASE_LEVEL)
            && (0 == key1_cooldown));
    key2_pressed = (uint8)((key2 == CAR_KEY_ACTIVE_LEVEL)
            && (key2_last == CAR_KEY_RELEASE_LEVEL)
            && (0 == key2_cooldown));
    key1_last = key1;
    key2_last = key2;

    if(key1_pressed != 0U)
    {
        key1_cooldown = CAR_KEY_COOLDOWN_COUNT;
        if(CAR_STATE_FOLLOW_RECORD == car_state) car_finish_record();
        else if((CAR_STATE_WAIT == car_state) || (CAR_STATE_FINISHED == car_state)
                || (CAR_STATE_ERROR == car_state) || (CAR_STATE_RECORD_FULL == car_state)
                || (CAR_STATE_PARK_STOP == car_state))
        {
            car_start_record();
        }
    }

    if(key2_pressed != 0U)
    {
        key2_cooldown = CAR_KEY_COOLDOWN_COUNT;
        if((CAR_STATE_WAIT == car_state) || (CAR_STATE_FINISHED == car_state)
                || (CAR_STATE_ERROR == car_state) || (CAR_STATE_PARK_STOP == car_state))
        {
            car_start_return();
        }
    }
#endif
}

void car_vision_set_command(float speed_mps, float steer_deg, uint8 valid, uint8 parking_reached)
{
    uint32 seq = car_vision_command_seq;
    if(seq & 1U) seq++;
    car_vision_command_seq = seq + 1U;
    __dsync();
    car_vision_command.speed_mps = speed_mps;
    car_vision_command.steer_deg = steer_deg;
    car_vision_command.valid = valid ? 1 : 0;
    car_vision_command.parking_reached = parking_reached ? 1 : 0;
    car_vision_command_ms = car_elapsed_ms;
    __dsync();
    car_vision_command_seq = seq + 2U;
}

static void car_apply_remote_manual(void)
{
    car_drive_direction = (car_remote_rear_pwm > 0) ? 1 : ((car_remote_rear_pwm < 0) ? -1 : 0);
    car_target_speed_mps = 0.0f;
    car_ramp_rear_pwm(car_remote_rear_pwm);
    car_drive_steer_count((float)car_remote_steer_target);
}

static uint8 car_read_vision_command(car_follow_input_struct *input)
{
    car_vision_command_struct command;
    uint32 seq_before;
    uint32 seq_after;
    uint32 command_ms;

    if(NULL == input) return 0U;

    do
    {
        seq_before = car_vision_command_seq;
        if(seq_before & 1U) return 0U;
        __dsync();
        command = car_vision_command;
        command_ms = car_vision_command_ms;
        __dsync();
        seq_after = car_vision_command_seq;
    }while((seq_before != seq_after) || (seq_after & 1U));

    input->speed_mps = command.speed_mps;
    input->steer_deg = command.steer_deg;
    input->valid = command.valid;
    input->parking_reached = command.parking_reached;
    input->age_ms = car_elapsed_ms - command_ms;
    return 1U;
}

static void car_update_follow_record(void)
{
    car_path_record_update(car_delta_s_m, car_yaw_unwrapped_deg,
            car_steer_measured_center_deg(), car_elapsed_ms);
    if(car_path_full)
    {
        car_error_code = CAR_ERROR_RECORD_FULL;
        car_all_motor_stop();
        car_enter_state(CAR_STATE_RECORD_FULL);
        return;
    }

#if CAR_REMOTE_ENABLE
    if(car_remote_active)
    {
        car_apply_remote_manual();
    }
    else
#endif
    {
        car_follow_input_struct input;
        car_follow_output_struct output;

        if(0U == car_read_vision_command(&input))
        {
            car_all_motor_stop();
            return;
        }
        if(input.parking_reached)
        {
            car_all_motor_stop();
            car_enter_state(CAR_STATE_PARK_STOP);
            return;
        }

        car_follow_update(&input, car_speed_mps, &output);
        car_drive_direction = output.driving ? 1 : 0;
        car_target_speed_mps = output.target_speed_mps;
        car_ramp_rear_pwm_turn(output.rear_pwm, output.steer_deg);
        if(!input.valid || (input.age_ms > CAR_FOLLOW_COMMAND_TIMEOUT_MS))
        {
            // Losing the tag stops steering where it is; it must not command center.
            car_set_steer_pwm(0);
        }
        else
        {
            car_drive_steer_angle(output.steer_deg);
        }
    }
}

static void car_update_return(void)
{
    car_replay_output_struct output;
    car_replay_status_enum status;

    status = car_replay_update(car_delta_s_m, car_yaw_unwrapped_deg,
            car_gyro_z_dps, car_speed_mps, &output);
    if(CAR_REPLAY_FINISHED == status)
    {
        car_finish_run();
        return;
    }
    if(CAR_REPLAY_RUNNING != status)
    {
        car_error_code = (CAR_REPLAY_HEADING_ERROR == status)
                ? CAR_ERROR_REPLAY_HEADING : CAR_ERROR_PATH_SHORT;
        car_all_motor_stop();
        car_enter_state(CAR_STATE_ERROR);
        return;
    }

    car_heading_error_deg = output.heading_error_deg;
    car_replay_remaining_m = output.remaining_m;
    car_replay_curvature = output.curvature;
    car_target_speed_mps = output.target_speed_mps;
    car_drive_direction = -1;
    car_ramp_rear_pwm_turn(output.rear_pwm, output.steer_deg);
    car_drive_steer_angle(output.steer_deg);
}

void car_nav_init(void)
{
    car_state = CAR_STATE_SELF_TEST;
    car_error_code = CAR_ERROR_NONE;
    car_elapsed_ms = 0;

    car_hardware_init();
    car_all_motor_stop();
    if(CAR_ERROR_ENCODER == car_error_code)
    {
        car_enter_state(CAR_STATE_ERROR);
        return;
    }

    if(0 != car_imu_init())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
        car_enter_state(CAR_STATE_ERROR);
        return;
    }
    if(0 != car_calibrate_gyro_bias())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
        car_enter_state(CAR_STATE_ERROR);
        return;
    }

#if CAR_REMOTE_ENABLE && CAR_REMOTE_LINK_UART
    uart_init(CAR_REMOTE_UART_INDEX, CAR_REMOTE_UART_BAUD,
            CAR_REMOTE_UART_TX_PIN, CAR_REMOTE_UART_RX_PIN);
#endif
    car_reset_pose();
#if CAR_STEER_BOOT_SELF_TEST_ENABLE
    car_steer_boot_self_test();
    car_reset_pose();
#endif
#if CAR_PATH_SAVE_TO_FLASH
    (void)car_path_flash_load();
#else
    car_path_clear();
#endif
#if CAR_AUTO_START_FOLLOW
    car_start_record();
#else
    car_enter_state(CAR_STATE_WAIT);
#endif
}

void car_nav_control_10ms(void)
{
    if(car_elapsed_ms < 0xFFFFFF00u) car_elapsed_ms += CAR_CONTROL_PERIOD_MS;
    car_update_keys();
#if CAR_REMOTE_ENABLE
    car_remote_apply_pending_commands();
#endif
    car_sensor_update();

#if CAR_REMOTE_ENABLE
    if((car_elapsed_ms - last_remote_ms) > CAR_REMOTE_TIMEOUT_MS)
    {
        car_remote_active = 0;
    }

    if(car_remote_active && (CAR_STATE_RETURN != car_state))
    {
        if(CAR_STATE_FOLLOW_RECORD == car_state)
        {
            car_update_follow_record();
        }
        else
        {
            car_apply_remote_manual();
        }
        return;
    }
#endif

    switch(car_state)
    {
        case CAR_STATE_WAIT:
        case CAR_STATE_FINISHED:
            if(ready_center_active) car_update_ready_center();
            else car_all_motor_stop();
            break;

        case CAR_STATE_ERROR:
        case CAR_STATE_RECORD_FULL:
            car_all_motor_stop();
            break;

        case CAR_STATE_FOLLOW_RECORD:
            car_update_follow_record();
            break;

        case CAR_STATE_PARK_STOP:
            car_all_motor_stop();
            break;

        case CAR_STATE_RETURN:
            car_update_return();
            break;

        default:
            car_all_motor_stop();
            break;
    }
}

void car_nav_debug_steer_init(void)
{
    car_state = CAR_STATE_WAIT;
    car_error_code = CAR_ERROR_NONE;
    car_elapsed_ms = 0;

    car_hardware_init();
    car_all_motor_stop();
    car_reset_pose();
    car_pid_reset(&steer_pid);
    steer_target_deg_smooth = 0.0f;
    rear_pwm_smooth = 0.0f;
    car_steer_decoder_reset();
#if CAR_STEER_SENSOR_TYPE != CAR_STEER_SENSOR_ABSOLUTE_SPI
    encoder_clear_count(CAR_STEER_ENCODER);
#endif

    if(0 != car_imu_init())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
    }
    else if(0 != car_calibrate_gyro_bias())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
    }
}

void car_nav_encoder_test_init(void)
{
    car_state = CAR_STATE_WAIT;
    car_error_code = CAR_ERROR_NONE;
    car_elapsed_ms = 0;

    car_hardware_init();
    car_all_motor_stop();
    car_reset_pose();
    car_pid_reset(&steer_pid);
    steer_target_deg_smooth = 0.0f;
    rear_pwm_smooth = 0.0f;
    car_left_rear_encoder_count = 0;
    car_right_rear_encoder_count = 0;
    car_steer_decoder_reset();
    encoder_clear_count(CAR_LEFT_REAR_ENCODER);
    encoder_clear_count(CAR_RIGHT_REAR_ENCODER);
#if CAR_STEER_SENSOR_TYPE != CAR_STEER_SENSOR_ABSOLUTE_SPI
    encoder_clear_count(CAR_STEER_ENCODER);
#endif

    if(0 != car_imu_init())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
    }
    else if(0 != car_calibrate_gyro_bias())
    {
        car_error_code = CAR_ERROR_IMU_LOST;
    }
}

void car_nav_encoder_test_10ms(void)
{
    if(car_elapsed_ms < 0xFFFFFF00u) car_elapsed_ms += CAR_CONTROL_PERIOD_MS;
    car_sensor_update();
#if CAR_USE_BOARD_KEYS
    car_key1_level = gpio_get_level(CAR_GUIDE_KEY_PIN);
    car_key2_level = gpio_get_level(CAR_RESET_KEY_PIN);
#endif

    car_all_motor_stop();
    car_drive_direction = 0;
    car_target_speed_mps = 0.0f;
    car_target_steer_deg = 0.0f;
}

void car_nav_debug_steer_10ms(void)
{
    float target_count;

    if(car_elapsed_ms < 0xFFFFFF00u) car_elapsed_ms += CAR_CONTROL_PERIOD_MS;
    car_update_keys();
    car_sensor_update();

    car_set_rear_pwm(0);
    car_drive_direction = 0;
    car_target_speed_mps = 0.0f;

    if((car_error_code == CAR_ERROR_NONE) && (car_vision_runtime_valid != 0U))
    {
        target_count = (float)car_vision_runtime_error_x_px * CAR_STEER_DEBUG_COUNT_PER_PIXEL;
    }
    else
    {
        target_count = 0.0f;
    }
    target_count = car_limitf(target_count,
            -CAR_STEER_ENC_MAX_ABS_COUNT,
            CAR_STEER_ENC_MAX_ABS_COUNT);
    car_drive_steer_count(target_count);
}
