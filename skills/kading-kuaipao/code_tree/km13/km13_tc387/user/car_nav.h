#ifndef _car_nav_h_
#define _car_nav_h_

#include "car_config.h"
#include "car_path.h"

typedef enum
{
    CAR_STATE_SELF_TEST = 0,
    CAR_STATE_WAIT,
    CAR_STATE_FOLLOW_RECORD,
    CAR_STATE_PARK_STOP,
    CAR_STATE_RETURN,
    CAR_STATE_FINISHED,
    CAR_STATE_RECORD_FULL,
    CAR_STATE_ERROR,
} car_state_enum;

typedef struct
{
    float kp;
    float ki;
    float kd;
    float integral_limit;
    float output_limit;
    float error;
    float last_error;
    float integral;
} car_pid_struct;

typedef struct
{
    float speed_mps;
    float steer_deg;
    uint8 valid;
    uint8 parking_reached;
} car_vision_command_struct;

extern volatile car_state_enum car_state;
extern volatile uint32 car_elapsed_ms;
extern volatile uint8 car_nav_10ms_pending;
extern volatile uint8 car_key1_level;
extern volatile uint8 car_key2_level;
extern volatile uint8 car_error_code;

extern volatile int32 car_left_rear_encoder_count;
extern volatile int32 car_right_rear_encoder_count;
extern volatile int16 car_left_rear_encoder_delta;
extern volatile int16 car_right_rear_encoder_delta;
extern volatile int32 car_steer_encoder_count;
extern volatile int16 car_steer_encoder_raw_delta;
extern volatile uint32 car_steer_encoder_reject_count;
extern volatile int16 car_steer_abs_zero_raw;
extern volatile int16 car_steer_abs_raw;
extern volatile float car_steer_angle_raw_deg;
extern volatile float car_steer_angle_deg;
extern volatile float car_yaw_deg;
extern volatile float car_yaw_unwrapped_deg;
extern volatile float car_speed_mps;
extern volatile float car_delta_s_m;
extern volatile float car_gyro_z_dps;
extern volatile float car_gyro_z_bias;
extern volatile float car_heading_error_deg;
extern volatile float car_replay_remaining_m;
extern volatile float car_target_speed_mps;
extern volatile float car_target_steer_deg;
extern volatile int16 car_target_steer_raw;
extern volatile float car_replay_curvature;
extern volatile int16 car_rear_pwm_output;
extern volatile int16 car_steer_pwm_output;
extern volatile int8 car_steer_stall_block_sign;
extern volatile int8 car_drive_direction;
extern volatile uint8 car_remote_active;
extern volatile uint32 car_remote_rx_count;
extern volatile uint32 car_remote_ok_count;
extern volatile int16 car_remote_rear_pwm;
extern volatile int16 car_remote_steer_target;
extern volatile uint8 car_wifi_module_ready;
extern volatile uint8 car_wifi_link_ready;
extern volatile uint8 car_wifi_stage;
extern volatile uint8 car_wifi_error;

void  car_nav_init             (void);
void  car_nav_control_10ms     (void);
void  car_nav_encoder_test_init(void);
void  car_nav_encoder_test_10ms(void);
void  car_nav_debug_steer_init (void);
void  car_nav_debug_steer_10ms (void);
void  car_remote_uart_callback (void);
void  car_remote_wifi_task     (void);
void  car_vision_set_command   (float speed_mps, float steer_deg, uint8 valid, uint8 parking_reached);
float car_pid_calc             (car_pid_struct *pid, float target, float current);
void  car_all_motor_stop       (void);

#endif
