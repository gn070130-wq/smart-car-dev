#ifndef _car_replay_h_
#define _car_replay_h_

#include "car_path.h"
#include "car_replay_config.h"

typedef enum
{
    CAR_REPLAY_RUNNING = 0,
    CAR_REPLAY_FINISHED,
    CAR_REPLAY_PATH_INVALID,
    CAR_REPLAY_HEADING_ERROR,
} car_replay_status_enum;

typedef struct
{
    int16 rear_pwm;
    float steer_deg;
    float target_speed_mps;
    float target_yaw_deg;
    float heading_error_deg;
    float remaining_m;
    float curvature;
    uint32 path_index;
} car_replay_output_struct;

extern volatile float car_replay_s_m;
extern volatile uint32 car_replay_index;

void car_replay_reset(void);
car_replay_status_enum car_replay_update(float delta_s_m, float yaw_deg,
        float yaw_rate_dps, float measured_speed_mps, car_replay_output_struct *output);

#endif
