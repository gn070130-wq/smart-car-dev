#ifndef _car_path_h_
#define _car_path_h_

#include "car_path_config.h"

typedef struct
{
    float s_m;
    float yaw_deg;
    float steer_deg;
    uint16 time_10ms;
} car_path_point_struct;

typedef struct
{
    float yaw_deg;
    float steer_deg;
    float curvature;
    float curvature_short;
    float curvature_long;
    float yaw_change_short_deg;
    float yaw_change_long_deg;
    uint32 lower_index;
} car_path_sample_struct;

extern volatile uint32 car_path_count;
extern volatile float car_record_s_m;
extern volatile uint32 car_path_duration_ms;
extern volatile uint8 car_path_full;
extern volatile uint8 car_path_saved_valid;

void car_path_clear(void);
void car_path_record_begin(float yaw_deg, float steer_deg, uint32 now_ms);
void car_path_record_update(float delta_s_m, float yaw_deg, float steer_deg, uint32 now_ms);
void car_path_record_finish(float yaw_deg, float steer_deg, uint32 now_ms);
uint8 car_path_is_ready(void);
uint8 car_path_flash_load(void);
uint8 car_path_flash_save(void);
float car_path_total_length(void);
uint8 car_path_sample_yaw(float s_m, float *yaw_deg, uint32 *lower_index);
uint8 car_path_sample(float s_m, car_path_sample_struct *sample);

#endif
