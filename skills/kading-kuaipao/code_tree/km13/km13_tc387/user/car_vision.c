#include "zf_common_headfile.h"
#include "car_nav.h"
#include "car_vision.h"
#include "car_follow_config.h"

#if CAR_VISION_ENABLE
#include "../code/vision_tag_tracker.h"
#endif

volatile uint8 car_vision_runtime_ready = 0;
volatile uint8 car_vision_runtime_valid = 0;
volatile uint8 car_vision_runtime_detected = 0;
volatile uint8 car_vision_runtime_predicted = 0;
volatile int16 car_vision_runtime_error_x_px = 0;
volatile int16 car_vision_runtime_error_y_px = 0;
volatile int16 car_vision_runtime_error_x_q15 = 0;
volatile uint16 car_vision_runtime_size_px = 0;
volatile uint16 car_vision_runtime_confidence = 0;
volatile uint16 car_vision_runtime_distance_mm = 0;
volatile uint32 car_vision_runtime_frame_id = 0;
volatile uint32 car_vision_runtime_process_us = 0;
volatile uint32 car_vision_runtime_fps_x10 = 0;
volatile int16 car_vision_runtime_bbox_x = 0;
volatile int16 car_vision_runtime_bbox_y = 0;
volatile int16 car_vision_runtime_bbox_w = 0;
volatile int16 car_vision_runtime_bbox_h = 0;

#if CAR_VISION_ENABLE
static uint8 park_confirm_frames = 0;
static uint8 follow_drive_latched = 0;
static uint32 last_good_ms = 0U;
static float last_good_speed_mps = 0.0f;
static float last_good_steer_deg = 0.0f;

static float vision_limitf(float value, float low, float high)
{
    if(value < low) return low;
    if(value > high) return high;
    return value;
}

static float vision_speed_from_result(const vision_tag_result_t *result)
{
    float speed = CAR_VISION_TARGET_SPEED_MPS;
    float slow_ratio;

    if((result->distance_mm != 0U)
            && (CAR_VISION_FOLLOW_NEAR_MM != 0U)
            && (CAR_VISION_FOLLOW_FAR_MM > CAR_VISION_FOLLOW_NEAR_MM))
    {
        if(result->distance_mm <= CAR_VISION_FOLLOW_NEAR_MM)
        {
            follow_drive_latched = 0U;
        }
        else if(result->distance_mm > CAR_VISION_FOLLOW_FAR_MM)
        {
            follow_drive_latched = 1U;
        }
        if(!follow_drive_latched)
        {
            return 0.0f;
        }
        if((CAR_VISION_FOLLOW_SLOW_MM > CAR_VISION_FOLLOW_NEAR_MM)
                && (result->distance_mm < CAR_VISION_FOLLOW_SLOW_MM))
        {
            slow_ratio = (float)(result->distance_mm - CAR_VISION_FOLLOW_NEAR_MM)
                    / (float)(CAR_VISION_FOLLOW_SLOW_MM - CAR_VISION_FOLLOW_NEAR_MM);
            slow_ratio = vision_limitf(slow_ratio, 0.0f, 1.0f);
            return CAR_VISION_NEAR_SPEED_MPS
                    + (CAR_VISION_TARGET_SPEED_MPS - CAR_VISION_NEAR_SPEED_MPS)
                    * slow_ratio;
        }
        return CAR_VISION_TARGET_SPEED_MPS;
    }

    if(result->size_px > 0U)
    {
        if(result->size_px <= CAR_VISION_FAR_SIZE_PX)
        {
            speed = CAR_VISION_FAR_SPEED_MPS;
        }
        else if(result->size_px >= CAR_VISION_NEAR_SIZE_PX)
        {
            speed = CAR_VISION_NEAR_SPEED_MPS;
        }
    }

    return vision_limitf(speed, CAR_VISION_SPEED_MIN_MPS, CAR_VISION_SPEED_MAX_MPS);
}

static float vision_steer_from_result(const vision_tag_result_t *result)
{
    float error_px = (float)result->error_x_px;
    float steer_deg;

    if(CAR_VISION_STEER_FULL_PX <= 0.0f)
    {
        return 0.0f;
    }

    steer_deg = error_px / CAR_VISION_STEER_FULL_PX
            * CAR_VISION_STEER_MAX_DEG * CAR_VISION_STEER_SIGN;
    steer_deg = vision_limitf(steer_deg, -CAR_VISION_STEER_MAX_DEG, CAR_VISION_STEER_MAX_DEG);
    return steer_deg;
}

static uint8 vision_parking_reached(const vision_tag_result_t *result)
{
#if CAR_VISION_AUTO_PARK_ENABLE
    if((result->valid != 0U) && (result->size_px >= CAR_VISION_PARK_SIZE_PX))
    {
        if(park_confirm_frames < 255U) park_confirm_frames++;
    }
    else
    {
        park_confirm_frames = 0U;
    }
    return (park_confirm_frames >= CAR_VISION_PARK_CONFIRM_FRAMES) ? 1U : 0U;
#else
    (void)result;
    park_confirm_frames = 0U;
    return 0U;
#endif
}
#endif

void car_vision_mark_camera_ready(uint8 ready)
{
    car_vision_runtime_ready = ready ? 1U : 0U;
}

void car_vision_publish_result(const vision_tag_result_t *result, uint32 process_us, uint32 fps_x10)
{
#if CAR_VISION_ENABLE
    uint8 usable;
    uint8 held = 0U;
    uint8 parking;
    float speed_mps = 0.0f;
    float steer_deg = 0.0f;

    if(result == NULL)
    {
        car_vision_set_command(0.0f, 0.0f, 0U, 0U);
        return;
    }

    car_vision_runtime_valid = result->valid;
    car_vision_runtime_detected = result->detected;
    car_vision_runtime_predicted = result->predicted;
    car_vision_runtime_error_x_px = result->error_x_px;
    car_vision_runtime_error_y_px = result->error_y_px;
    car_vision_runtime_error_x_q15 = result->error_x_q15;
    car_vision_runtime_size_px = result->size_px;
    car_vision_runtime_confidence = result->confidence;
    car_vision_runtime_distance_mm = result->distance_mm;
    car_vision_runtime_frame_id = result->frame_id;
    car_vision_runtime_process_us = process_us;
    car_vision_runtime_fps_x10 = fps_x10;
    car_vision_runtime_bbox_x = result->bbox.x;
    car_vision_runtime_bbox_y = result->bbox.y;
    car_vision_runtime_bbox_w = result->bbox.width;
    car_vision_runtime_bbox_h = result->bbox.height;

    usable = ((result->valid != 0U)
            && (result->confidence >= CAR_VISION_VALID_CONF_MIN)) ? 1U : 0U;
    if(usable != 0U)
    {
        speed_mps = vision_speed_from_result(result);
        steer_deg = vision_steer_from_result(result);
        last_good_ms = car_elapsed_ms;
        last_good_speed_mps = speed_mps;
        last_good_steer_deg = steer_deg;
    }
    else if((last_good_ms != 0U)
            && ((car_elapsed_ms - last_good_ms) <= CAR_VISION_LOST_HOLD_MS))
    {
        speed_mps = last_good_speed_mps * CAR_VISION_LOST_HOLD_SPEED_SCALE;
        steer_deg = last_good_steer_deg;
        usable = 1U;
        held = 1U;
    }

    parking = (held == 0U) ? vision_parking_reached(result) : 0U;
    car_vision_set_command(speed_mps, steer_deg, usable, parking);
#else
    (void)result;
    (void)process_us;
    (void)fps_x10;
    car_vision_set_command(0.0f, 0.0f, 0U, 0U);
#endif
}
