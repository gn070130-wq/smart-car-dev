#ifndef _car_follow_config_h_
#define _car_follow_config_h_

#include "car_config.h"

// Vision-to-follow conversion. These values never affect replay.
#define CAR_VISION_SPEED_MIN_MPS          (0.0f)
#define CAR_VISION_SPEED_MAX_MPS          (0.42f)
#define CAR_VISION_TARGET_SPEED_MPS       (0.23f)
#define CAR_VISION_FAR_SPEED_MPS          (0.28f)
#define CAR_VISION_NEAR_SPEED_MPS         (0.06f)
#define CAR_VISION_STEER_SIGN             (-1.0f)
#define CAR_VISION_STEER_MAX_DEG          (60.0f)
#define CAR_VISION_STEER_FULL_PX          (50.0f)
#define CAR_VISION_VALID_CONF_MIN         (260)
#define CAR_VISION_FAR_SIZE_PX            (22)
#define CAR_VISION_NEAR_SIZE_PX           (56)
#define CAR_VISION_DISTANCE_SCALE_MM_PX   (13200UL)
#define CAR_VISION_FOLLOW_NEAR_MM         (300U)
#define CAR_VISION_FOLLOW_SLOW_MM         (400U)
#define CAR_VISION_FOLLOW_FAR_MM          (450U)
#define CAR_VISION_AUTO_PARK_ENABLE       (0)
#define CAR_VISION_PARK_SIZE_PX           (72)
#define CAR_VISION_PARK_CONFIRM_FRAMES    (12)
#define CAR_VISION_LOST_HOLD_MS           (350)
#define CAR_VISION_LOST_HOLD_SPEED_SCALE  (0.65f)

// Follow controller only. Brief target loss is held by car_vision before this timeout.
#define CAR_FOLLOW_COMMAND_TIMEOUT_MS     (450)
#define CAR_FOLLOW_SPEED_DEADBAND_MPS     (0.025f)
#define CAR_FOLLOW_SPEED_SLEW_MPS_TICK    (0.018f)
#define CAR_FOLLOW_STEER_FILTER_ALPHA     (0.50f)
#define CAR_FOLLOW_PWM_PER_MPS            (8500.0f)
#define CAR_FOLLOW_PWM_MIN_MOVE           (1450)
#define CAR_FOLLOW_PWM_MIN_MOVE_SPEED_MPS (0.05f)
#define CAR_FOLLOW_SPEED_KP               (1800.0f)
#define CAR_FOLLOW_SPEED_KI               (650.0f)
#define CAR_FOLLOW_SPEED_I_LIMIT          (900.0f)

#endif
