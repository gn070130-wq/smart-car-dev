#ifndef _car_path_config_h_
#define _car_path_config_h_

#include "car_config.h"

// Distance/yaw keyframes. The random course can contain a complete 360 degree
// loop, so yaw is always stored unwrapped and curvature is fitted in distance.
#define CAR_PATH_MAX_POINTS               (900)
#define CAR_PATH_MIN_TOTAL_M              (0.80f)
#define CAR_PATH_MIN_POINTS               (3)
#define CAR_PATH_STRAIGHT_SPACING_M       (0.08f)
#define CAR_PATH_TURN_YAW_STEP_DEG        (5.0f)
#define CAR_PATH_TURN_MIN_MOVE_M          (0.025f)
#define CAR_PATH_FINAL_MIN_MOVE_M         (0.005f)
#define CAR_PATH_RECORD_MOVE_EPS_M         (0.00020f)
#define CAR_PATH_YAW_DEADBAND_DEG          (0.50f)
#define CAR_PATH_YAW_LOWPASS_ALPHA         (0.35f)
#define CAR_PATH_STEER_DEADBAND_DEG        (0.35f)
#define CAR_PATH_STEER_LOWPASS_ALPHA       (0.50f)
#define CAR_PATH_CURVATURE_SHORT_HALF_M    (0.12f)
#define CAR_PATH_CURVATURE_LONG_HALF_M     (0.30f)
#define CAR_PATH_CURVATURE_FIT_SAMPLES     (7U)
#define CAR_PATH_CURVATURE_MIN_WINDOW_M    (0.04f)

// DFlash pages 62/63 are reserved by vision calibration. KM13 route uses 48..55.
#define CAR_PATH_FLASH_PAGE_START         (48U)
#define CAR_PATH_FLASH_PAGE_COUNT         (8U)

#endif
