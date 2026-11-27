/*********************************************************************************************************************
 * TC387QP vision worker task.
 * CPU1 consumes immutable camera buffers, runs the tracker from PSPR1/DSPR1, and publishes a sequence-locked result.
 ********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "../code/vision_calibration_store.h"
#include "../code/vision_shared.h"
#include "../code/vision_tag_tracker.h"
#include "car_follow_config.h"
#include "car_nav.h"
#include "IfxStm.h"

#define VISION_EXPOSURE_MIN          (64U)
#define VISION_EXPOSURE_MAX          (768U)
/* Recover exposure rapidly after startup or a lighting transition. */
#define VISION_EXPOSURE_INTERVAL     (8U)
#define VISION_EXPOSURE_ENABLED      (0U)
/* Keep IPS traffic tiny: only the tuning parameters are shown. */
#define VISION_DISPLAY_INTERVAL_MS   (200U)
#define VISION_UI_Y0                 (0U)
#define VISION_UI_ROW                (22U)
#define VISION_UI_VALUE_X            (64U)

static void vision_ips_status_labels_show(void);

static void vision_debug_output_uart(const char *str)
{
    if(str != NULL)
    {
        debug_send_buffer((const uint8 *)str, (uint32)strlen(str));
    }
}

static void vision_ips_detach_debug_screen(void)
{
    debug_output_struct info;

    debug_output_struct_init(&info);
    info.output_uart = vision_debug_output_uart;
    debug_output_init(&info);
}

static int32 vision_round_float_to_i32(float value)
{
    return (int32)((value >= 0.0f) ? (value + 0.5f) : (value - 0.5f));
}

static const char *vision_state_text(void)
{
    switch(car_state)
    {
        case CAR_STATE_FOLLOW_RECORD: return "REC";
        case CAR_STATE_WAIT:          return "READY";
        case CAR_STATE_RETURN:        return "BACK";
        case CAR_STATE_PARK_STOP:     return "PARK";
        case CAR_STATE_FINISHED:      return "DONE";
        case CAR_STATE_ERROR:         return "ERR";
        case CAR_STATE_RECORD_FULL:   return "FULL";
        default:                      return "WAIT";
    }
}

static void vision_ips_clear_value_row(uint8 row)
{
    ips200_show_string(VISION_UI_VALUE_X, VISION_UI_Y0 + (uint16)row * VISION_UI_ROW, "          ");
}

static void vision_ips_clear_values(void)
{
    uint8 row;

    for(row = 0U; row < 10U; row++)
    {
        vision_ips_clear_value_row(row);
    }
}

static void vision_ips_status_init(void)
{
    ips200_set_font(IPS200_8X16_FONT);
    ips200_set_color(RGB565_RED, RGB565_BLACK);
    ips200_clear();
    vision_ips_status_labels_show();
}

static void vision_ips_status_labels_show(void)
{
    ips200_show_string(0U, VISION_UI_Y0, "ST:");
    ips200_show_string(0U, VISION_UI_Y0 + VISION_UI_ROW, "ERR:");
    ips200_show_string(0U, VISION_UI_Y0 + 2U * VISION_UI_ROW, "DX:");
    ips200_show_string(0U, VISION_UI_Y0 + 3U * VISION_UI_ROW, "DIST:");
    ips200_show_string(0U, VISION_UI_Y0 + 4U * VISION_UI_ROW, "REC:");
    ips200_show_string(0U, VISION_UI_Y0 + 5U * VISION_UI_ROW, "REM:");
    ips200_show_string(0U, VISION_UI_Y0 + 6U * VISION_UI_ROW, "PT:");
    ips200_show_string(0U, VISION_UI_Y0 + 7U * VISION_UI_ROW, "SA:");
    ips200_show_string(0U, VISION_UI_Y0 + 8U * VISION_UI_ROW, "YAW:");
    ips200_show_string(0U, VISION_UI_Y0 + 9U * VISION_UI_ROW, "SPWM:");
}

static void vision_ips_status_update(const vision_tag_result_t *result)
{
    int32 remaining_mm;

    ips200_set_color(RGB565_RED, RGB565_BLACK);
    vision_ips_status_labels_show();
    vision_ips_clear_values();
    (void)result;

    ips200_show_string(VISION_UI_VALUE_X, VISION_UI_Y0, vision_state_text());
    ips200_show_uint(VISION_UI_VALUE_X, VISION_UI_Y0 + VISION_UI_ROW,
                     car_error_code, 3U);
    if((result != NULL) && (result->valid != 0U))
    {
        ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 2U * VISION_UI_ROW,
                        result->error_x_px, 5U);
        ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 3U * VISION_UI_ROW,
                        result->distance_mm, 5U);
    }
    else
    {
        ips200_show_string(VISION_UI_VALUE_X, VISION_UI_Y0 + 2U * VISION_UI_ROW, "-----");
        ips200_show_string(VISION_UI_VALUE_X, VISION_UI_Y0 + 3U * VISION_UI_ROW, "-----");
    }

    remaining_mm = (car_state == CAR_STATE_RETURN)
            ? vision_round_float_to_i32(car_replay_remaining_m * 1000.0f)
            : vision_round_float_to_i32(car_record_s_m * 1000.0f);
    ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 4U * VISION_UI_ROW,
                    vision_round_float_to_i32(car_record_s_m * 1000.0f), 6U);
    ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 5U * VISION_UI_ROW,
                    remaining_mm, 6U);
    ips200_show_uint(VISION_UI_VALUE_X, VISION_UI_Y0 + 6U * VISION_UI_ROW,
                     car_path_count, 4U);
    ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 7U * VISION_UI_ROW,
                    car_steer_abs_raw, 5U);
    ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 8U * VISION_UI_ROW,
                    vision_round_float_to_i32(car_yaw_unwrapped_deg), 5U);
    ips200_show_int(VISION_UI_VALUE_X, VISION_UI_Y0 + 9U * VISION_UI_ROW,
                    car_steer_pwm_output, 6U);
}

static uint16 vision_exposure_next(const vision_tag_result_t *result, uint16 current)
{
    uint32 step;
    uint32 next = current;

    if ((result->saturated_permille >= 20U) || (result->gray_p90 >= 248U))
    {
        step = (current / 4U > 8U) ? current / 4U : 8U;
        next = (current > step) ? current - step : VISION_EXPOSURE_MIN;
    }
    else if (result->gray_p90 > 232U)
    {
        step = (current / 8U > 4U) ? current / 8U : 4U;
        next = (current > step) ? current - step : VISION_EXPOSURE_MIN;
    }
    else if ((result->gray_p90 < 205U) && (result->gray_p50 < 170U))
    {
        step = (current / 4U > 16U) ? current / 4U : 16U;
        next = current + step;
    }

    if (next < VISION_EXPOSURE_MIN)
    {
        next = VISION_EXPOSURE_MIN;
    }
    if (next > VISION_EXPOSURE_MAX)
    {
        next = VISION_EXPOSURE_MAX;
    }
    return (uint16)next;
}

void core1_main(void)
{
#if !CAR_VISION_ENABLE
    disable_Watchdog();
    interrupt_global_enable(0);
    cpu_wait_event_ready();

    while(TRUE)
    {
        system_delay_ms(1000U);
    }
#else
    const uint8 *camera_frame;
    const vision_tag_result_t *vision_result = NULL;
    vision_tag_config_t tracker_config;
    vision_calibration_t calibration;
    Ifx_STM *stm;
    uint32 camera_sequence = 0U;
#if CAR_DISPLAY_MODE
    uint32 display_window_start;
#endif
    uint32 stm_frequency;
    uint16 exposure = MT9V03X_EXP_TIME_DEF;

    disable_Watchdog();
    interrupt_global_enable(0);
#if CAR_DISPLAY_MODE
    ips200_init(CAR_IPS200_TYPE);
    vision_ips_detach_debug_screen();
    vision_ips_status_init();
#endif

    cpu_wait_event_ready();
    vision_tag_tracker_default_config(&tracker_config);
    tracker_config.distance_scale_mm_px = CAR_VISION_DISTANCE_SCALE_MM_PX;
    tracker_config.follow_near_mm = CAR_VISION_FOLLOW_NEAR_MM;
    tracker_config.follow_far_mm = CAR_VISION_FOLLOW_FAR_MM;
    if (vision_calibration_store_get(&calibration) != 0U)
    {
        tracker_config.distance_scale_mm_px = calibration.distance_scale_mm_px;
        tracker_config.follow_near_mm = calibration.follow_near_mm;
        tracker_config.follow_far_mm = calibration.follow_far_mm;
    }
    vision_tag_tracker_init(&tracker_config);
    stm = IfxStm_getAddress((IfxStm_Index)IfxCpu_getCoreId());
    stm_frequency = IfxStm_getFrequency(stm);
#if CAR_DISPLAY_MODE
    display_window_start = IfxStm_getLower(stm);
#endif

    while (TRUE)
    {
        camera_frame = mt9v03x_acquire_frame(&camera_sequence);
        if (camera_frame != NULL)
        {
            uint32 process_us;

            system_start();
            vision_result = vision_tag_tracker_process(camera_frame,
                                                       MT9V03X_W,
                                                       MT9V03X_H,
                                                       MT9V03X_W);
            process_us = system_getval_us();
            vision_shared_publish(vision_result, camera_sequence, process_us);

            mt9v03x_release_frame();

            if ((VISION_EXPOSURE_ENABLED != 0U)
                && ((vision_result->frame_id % VISION_EXPOSURE_INTERVAL) == 0U))
            {
                uint16 next_exposure = vision_exposure_next(vision_result, exposure);
                if (next_exposure != exposure)
                {
                    exposure = next_exposure;
                    vision_shared_request_exposure(exposure);
                }
            }
        }

#if CAR_DISPLAY_MODE
        {
            uint32 now_ticks = IfxStm_getLower(stm);
            if((now_ticks - display_window_start)
               >= (stm_frequency * VISION_DISPLAY_INTERVAL_MS / 1000U))
            {
                vision_ips_status_update(vision_result);
                display_window_start = now_ticks;
            }
        }
#endif
    }
#endif
}
