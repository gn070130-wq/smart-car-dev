#include "car_replay.h"
#include <math.h>

#pragma section all "cpu0_dsram"

volatile float car_replay_s_m = 0.0f;
volatile uint32 car_replay_index = 0;

static float replay_speed_integral = 0.0f;
static float replay_heading_integral = 0.0f;
static float replay_steer_filtered_deg = 0.0f;
static float replay_target_yaw_filtered_deg = 0.0f;
static uint8 replay_target_yaw_valid = 0U;
static float replay_yaw_offset_deg = 0.0f;
static uint8 replay_yaw_offset_valid = 0U;
static float replay_phase_offset_m = 0.0f;
static float replay_reference_s_m = 0.0f;
static uint8 replay_reference_valid = 0U;
static uint8 replay_turn_active = 0U;

static float replay_absf(float value)
{
    return (value >= 0.0f) ? value : -value;
}

static float replay_limitf(float value, float min_value, float max_value)
{
    if(value < min_value) return min_value;
    if(value > max_value) return max_value;
    return value;
}

static float replay_wrap_deg(float value)
{
    while(value > 180.0f) value -= 360.0f;
    while(value < -180.0f) value += 360.0f;
    return value;
}

static uint8 replay_same_curvature_sign(float a, float b)
{
    if((replay_absf(a) < CAR_REPLAY_SIGN_COMPARE_EPS)
            || (replay_absf(b) < CAR_REPLAY_SIGN_COMPARE_EPS))
    {
        return 1U;
    }
    return ((a > 0.0f) == (b > 0.0f)) ? 1U : 0U;
}

static uint8 replay_path_has_turn(const car_path_sample_struct *sample)
{
    if(sample == NULL) return 0U;
    return ((replay_absf(sample->curvature_short) >= CAR_REPLAY_CURVE_MIN_CURVATURE)
            || (replay_absf(sample->curvature_long) >= CAR_REPLAY_CURVE_MIN_CURVATURE)
            || (replay_absf(sample->steer_deg) >= CAR_REPLAY_RECORDED_STEER_MIN_DEG)
            || (replay_absf(sample->yaw_change_long_deg)
                >= CAR_REPLAY_TURN_ENTER_LONG_YAW_DEG)) ? 1U : 0U;
}

static float replay_select_curvature(const car_path_sample_struct *sample)
{
    float short_k;
    float long_k;

    if(sample == NULL) return 0.0f;
    short_k = sample->curvature_short;
    long_k = sample->curvature_long;

    // Opposite signs mean an S-bend inflection is inside the long window.
    // Follow the short window so the two bends cannot cancel each other.
    if(!replay_same_curvature_sign(short_k, long_k))
    {
        return (replay_absf(short_k) >= CAR_REPLAY_SIGN_COMPARE_EPS)
                ? short_k : 0.0f;
    }
    if(replay_absf(short_k) >= CAR_REPLAY_SHORT_SELECT_CURVATURE)
    {
        return short_k;
    }
    if((replay_absf(sample->yaw_change_long_deg) >= CAR_REPLAY_TURN_ENTER_LONG_YAW_DEG)
            || (replay_absf(long_k) >= CAR_REPLAY_CURVE_MIN_CURVATURE))
    {
        return 0.30f * short_k + 0.70f * long_k;
    }
    if(replay_turn_active
            && (replay_absf(short_k) >= CAR_REPLAY_SIGN_COMPARE_EPS))
    {
        return short_k;
    }
    return 0.0f;
}

static float replay_phase_aligned_s(float encoder_s_m, float yaw_deg,
        float total_m, uint8 enable)
{
    float desired_offset = replay_phase_offset_m;
    float best_cost = 1.0e30f;
    float offset;

    if(enable)
    {
        for(offset = -CAR_REPLAY_PHASE_SEARCH_RANGE_M;
                offset <= CAR_REPLAY_PHASE_SEARCH_RANGE_M + 0.0001f;
                offset += CAR_REPLAY_PHASE_SEARCH_STEP_M)
        {
            float candidate_s = encoder_s_m + offset;
            float candidate_yaw;
            float yaw_error;
            float distance_penalty;
            float cost;

            if(candidate_s < 0.0f) candidate_s = 0.0f;
            if(candidate_s > total_m) candidate_s = total_m;
            if(replay_reference_valid
                    && (candidate_s > replay_reference_s_m
                        + CAR_REPLAY_REFERENCE_BACKSTEP_M))
            {
                continue;
            }
            if(!car_path_sample_yaw(candidate_s, &candidate_yaw, NULL)) continue;

            // Do not wrap this error. Unwrapped yaw distinguishes the entry
            // and exit of a complete 360 degree loop at the same location.
            yaw_error = candidate_yaw + replay_yaw_offset_deg - yaw_deg;
            distance_penalty = (candidate_s - encoder_s_m)
                    * CAR_REPLAY_PHASE_DISTANCE_DEG_PER_M;
            cost = yaw_error * yaw_error + distance_penalty * distance_penalty;
            if(cost < best_cost)
            {
                best_cost = cost;
                desired_offset = candidate_s - encoder_s_m;
            }
        }
    }

    if(desired_offset - replay_phase_offset_m > CAR_REPLAY_PHASE_MAX_STEP_M)
    {
        desired_offset = replay_phase_offset_m + CAR_REPLAY_PHASE_MAX_STEP_M;
    }
    if(desired_offset - replay_phase_offset_m < -CAR_REPLAY_PHASE_MAX_STEP_M)
    {
        desired_offset = replay_phase_offset_m - CAR_REPLAY_PHASE_MAX_STEP_M;
    }
    replay_phase_offset_m += (desired_offset - replay_phase_offset_m)
            * CAR_REPLAY_PHASE_FILTER_ALPHA;
    replay_phase_offset_m = replay_limitf(replay_phase_offset_m,
            -CAR_REPLAY_PHASE_SEARCH_RANGE_M, CAR_REPLAY_PHASE_SEARCH_RANGE_M);

    encoder_s_m += replay_phase_offset_m;
    encoder_s_m = replay_limitf(encoder_s_m, 0.0f, total_m);
    if(replay_reference_valid && (encoder_s_m > replay_reference_s_m))
    {
        encoder_s_m = replay_reference_s_m;
    }
    replay_reference_s_m = encoder_s_m;
    replay_reference_valid = 1U;
    return encoder_s_m;
}

void car_replay_reset(void)
{
    car_replay_s_m = 0.0f;
    car_replay_index = (car_path_count > 0U) ? (car_path_count - 1U) : 0U;
    replay_speed_integral = 0.0f;
    replay_heading_integral = 0.0f;
    replay_steer_filtered_deg = 0.0f;
    replay_target_yaw_filtered_deg = 0.0f;
    replay_target_yaw_valid = 0U;
    replay_yaw_offset_deg = 0.0f;
    replay_yaw_offset_valid = 0U;
    replay_phase_offset_m = 0.0f;
    replay_reference_s_m = 0.0f;
    replay_reference_valid = 0U;
    replay_turn_active = 0U;
}

car_replay_status_enum car_replay_update(float delta_s_m, float yaw_deg,
        float yaw_rate_dps, float measured_speed_mps, car_replay_output_struct *output)
{
    car_path_sample_struct encoder_sample;
    car_path_sample_struct sample;
    float end_yaw_deg;
    float total_m;
    float remaining_m;
    float reference_s_m;
    float sample_s_m;
    float speed_abs_mps;
    float speed_error;
    float pwm_abs;
    float curvature;
    float feedforward_deg;
    float curvature_feedforward_deg;
    float desired_yaw_rate_dps;
    float steer_deg;
    float target_yaw_error_deg;
    uint8 steer_control_active;
    uint8 recorded_straight;

    if(NULL == output) return CAR_REPLAY_PATH_INVALID;
    if(!car_path_is_ready()) return CAR_REPLAY_PATH_INVALID;

    // Forward wheel jitter or manual movement must never advance a reverse run.
    if(delta_s_m < 0.0f) car_replay_s_m += -delta_s_m;
    total_m = car_path_total_length();
    remaining_m = total_m - car_replay_s_m;
    if(remaining_m <= CAR_REPLAY_STOP_MARGIN_M)
    {
        output->rear_pwm = 0;
        output->steer_deg = 0.0f;
        output->remaining_m = 0.0f;
        return CAR_REPLAY_FINISHED;
    }

    if(!replay_yaw_offset_valid)
    {
        float start_error_deg;
        if(!car_path_sample_yaw(total_m, &end_yaw_deg, NULL))
        {
            return CAR_REPLAY_PATH_INVALID;
        }
        start_error_deg = replay_wrap_deg(end_yaw_deg - yaw_deg);
        if(replay_absf(start_error_deg) > CAR_REPLAY_START_HEADING_MAX_DEG)
        {
            output->rear_pwm = 0;
            output->steer_deg = 0.0f;
            output->remaining_m = remaining_m;
            return CAR_REPLAY_HEADING_ERROR;
        }
        replay_yaw_offset_deg = yaw_deg - end_yaw_deg;
        replay_yaw_offset_valid = 1U;
        replay_reference_s_m = total_m;
        replay_reference_valid = 1U;
    }

    if(!car_path_sample(remaining_m, &encoder_sample))
    {
        return CAR_REPLAY_PATH_INVALID;
    }
    if(replay_path_has_turn(&encoder_sample)) replay_turn_active = 1U;
    reference_s_m = replay_phase_aligned_s(remaining_m, yaw_deg, total_m,
            replay_turn_active);

    sample_s_m = reference_s_m - CAR_REPLAY_LOOKAHEAD_M;
    if(sample_s_m < 0.0f) sample_s_m = 0.0f;
    if(!car_path_sample(sample_s_m, &sample)) return CAR_REPLAY_PATH_INVALID;
    if(replay_path_has_turn(&sample)) replay_turn_active = 1U;

    if(!replay_target_yaw_valid)
    {
        replay_target_yaw_filtered_deg = sample.yaw_deg + replay_yaw_offset_deg;
        replay_target_yaw_valid = 1U;
    }
    else
    {
        // Both values are continuous. Keeping this error unwrapped preserves
        // every full turn instead of reducing 360 degrees to zero.
        target_yaw_error_deg = sample.yaw_deg + replay_yaw_offset_deg
                - replay_target_yaw_filtered_deg;
        if(replay_absf(target_yaw_error_deg) > CAR_REPLAY_TARGET_YAW_DEADBAND_DEG)
        {
            replay_target_yaw_filtered_deg += target_yaw_error_deg
                    * CAR_REPLAY_TARGET_YAW_FILTER_ALPHA;
        }
    }

    output->heading_error_deg = replay_wrap_deg(replay_target_yaw_filtered_deg - yaw_deg);
    if(replay_absf(output->heading_error_deg) > CAR_REPLAY_HEADING_ABORT_DEG)
    {
        output->rear_pwm = 0;
        output->steer_deg = 0.0f;
        output->remaining_m = remaining_m;
        return CAR_REPLAY_HEADING_ERROR;
    }

    if(replay_turn_active
            && (replay_absf(sample.curvature_short) <= CAR_REPLAY_TURN_EXIT_CURVATURE)
            && (replay_absf(sample.curvature_long) <= CAR_REPLAY_TURN_EXIT_CURVATURE)
            && (replay_absf(sample.steer_deg)
                < CAR_REPLAY_RECORDED_STEER_MIN_DEG)
            && (replay_absf(sample.yaw_change_long_deg)
                <= CAR_REPLAY_TURN_EXIT_LONG_YAW_DEG)
            && (replay_absf(output->heading_error_deg)
                <= CAR_REPLAY_STRAIGHT_HEADING_MAX_DEG)
            && (replay_absf(yaw_rate_dps)
                <= CAR_REPLAY_STRAIGHT_YAW_RATE_MAX_DPS))
    {
        replay_turn_active = 0U;
    }

    curvature = replay_limitf(replay_select_curvature(&sample)
            * CAR_REPLAY_CURVATURE_GAIN,
            -CAR_REPLAY_CURVATURE_LIMIT, CAR_REPLAY_CURVATURE_LIMIT);
    curvature_feedforward_deg = (float)atan((double)(CAR_WHEELBASE_M * curvature))
            * CAR_RAD_TO_DEG;
    recorded_straight = (replay_absf(sample.steer_deg)
            < CAR_REPLAY_RECORDED_STEER_MIN_DEG) ? 1U : 0U;
    if(!replay_turn_active
            && (replay_absf(curvature) <= CAR_REPLAY_STRAIGHT_CURVATURE_MAX)
            && recorded_straight
            && (replay_absf(output->heading_error_deg)
                <= CAR_REPLAY_STRAIGHT_LOCK_HEADING_DEG)
            && (replay_absf(yaw_rate_dps) <= CAR_REPLAY_STRAIGHT_YAW_RATE_MAX_DPS))
    {
        // A straight recorded segment must hold the physical steering center.
        steer_deg = 0.0f;
        replay_steer_filtered_deg = 0.0f;
        feedforward_deg = 0.0f;
        steer_control_active = 0U;
        replay_heading_integral *= 0.80f;
    }
    else
    {
        feedforward_deg = recorded_straight
                ? curvature_feedforward_deg
                : sample.steer_deg * CAR_REPLAY_RECORDED_STEER_GAIN;
        steer_control_active = 1U;
        if(replay_absf(output->heading_error_deg)
                > CAR_REPLAY_STRAIGHT_LOCK_HEADING_DEG)
        {
            replay_heading_integral += output->heading_error_deg * CAR_CONTROL_PERIOD_S;
            replay_heading_integral = replay_limitf(replay_heading_integral,
                    -CAR_REPLAY_HEADING_I_LIMIT_DEG, CAR_REPLAY_HEADING_I_LIMIT_DEG);
        }
        else
        {
            replay_heading_integral *= 0.95f;
        }
    }

    speed_abs_mps = CAR_REPLAY_TARGET_SPEED_MPS;
    if(remaining_m < CAR_REPLAY_SLOWDOWN_DISTANCE_M)
    {
        speed_abs_mps *= remaining_m / CAR_REPLAY_SLOWDOWN_DISTANCE_M;
        if(speed_abs_mps < CAR_REPLAY_MIN_SPEED_MPS) speed_abs_mps = CAR_REPLAY_MIN_SPEED_MPS;
    }

    if(steer_control_active)
    {
        desired_yaw_rate_dps = -speed_abs_mps * curvature * CAR_RAD_TO_DEG;

        // Reverse motion inverts heading feedback: positive yaw error needs negative steering.
        steer_deg = feedforward_deg
                - CAR_REPLAY_HEADING_KP * output->heading_error_deg
                - CAR_REPLAY_HEADING_KI * replay_heading_integral
                - CAR_REPLAY_YAW_RATE_KD * (desired_yaw_rate_dps - yaw_rate_dps);
        steer_deg *= CAR_REPLAY_STEER_SIGN;
        steer_deg = replay_limitf(steer_deg,
                -CAR_STEER_TARGET_LIMIT_DEG, CAR_STEER_TARGET_LIMIT_DEG);
        replay_steer_filtered_deg += (steer_deg - replay_steer_filtered_deg)
                * CAR_REPLAY_STEER_FILTER_ALPHA;
    }

    speed_error = speed_abs_mps - replay_absf(measured_speed_mps);
    replay_speed_integral += speed_error * CAR_REPLAY_SPEED_KI * CAR_CONTROL_PERIOD_S;
    replay_speed_integral = replay_limitf(replay_speed_integral,
            -CAR_REPLAY_SPEED_I_LIMIT, CAR_REPLAY_SPEED_I_LIMIT);
    pwm_abs = speed_abs_mps * CAR_REPLAY_PWM_PER_MPS
            + speed_error * CAR_REPLAY_SPEED_KP + replay_speed_integral;
    if((speed_abs_mps >= CAR_REPLAY_MIN_SPEED_MPS)
            && (pwm_abs > 0.0f)
            && (pwm_abs < (float)CAR_REPLAY_PWM_MIN_MOVE))
    {
        pwm_abs = (float)CAR_REPLAY_PWM_MIN_MOVE;
    }
    pwm_abs = replay_limitf(pwm_abs, 0.0f, (float)CAR_REAR_DUTY_LIMIT);

    car_replay_index = sample.lower_index;
    output->rear_pwm = -(int16)pwm_abs;
    output->steer_deg = replay_steer_filtered_deg;
    output->target_speed_mps = -speed_abs_mps;
    output->target_yaw_deg = replay_target_yaw_filtered_deg;
    output->remaining_m = remaining_m;
    output->curvature = curvature;
    output->path_index = sample.lower_index;
    return CAR_REPLAY_RUNNING;
}

#pragma section all restore
