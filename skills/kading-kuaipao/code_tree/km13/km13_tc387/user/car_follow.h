#ifndef _car_follow_h_
#define _car_follow_h_

#include "car_follow_config.h"

typedef struct
{
    float speed_mps;
    float steer_deg;
    uint8 valid;
    uint8 parking_reached;
    uint32 age_ms;
} car_follow_input_struct;

typedef struct
{
    int16 rear_pwm;
    float steer_deg;
    float target_speed_mps;
    uint8 driving;
} car_follow_output_struct;

void car_follow_reset(void);
void car_follow_update(const car_follow_input_struct *input,
        float measured_speed_mps, car_follow_output_struct *output);

#endif
