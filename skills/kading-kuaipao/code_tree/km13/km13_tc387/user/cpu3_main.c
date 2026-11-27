/*
 * KM13 chassis task.
 * CPU0/CPU1 keep the reference vision pipeline intact. CPU3 only consumes
 * published vision results and runs follow/record/replay control.
 */
#include "zf_common_headfile.h"
#include "car_config.h"
#include "car_nav.h"
#include "car_vision.h"
#include "../code/vision_shared.h"

#pragma section all "cpu3_dsram"

static void km13_vision_command_poll(void)
{
#if CAR_VISION_ENABLE
    static uint32 last_frame_id = 0U;
    static uint32 fps_window_ms = 0U;
    static uint32 fps_window_frames = 0U;
    static uint32 fps_x10 = 0U;
    vision_runtime_snapshot_t snapshot;

    if((vision_shared_read(&snapshot) != 0U)
            && (snapshot.result.frame_id != last_frame_id))
    {
        last_frame_id = snapshot.result.frame_id;
        if(fps_window_ms == 0U)
        {
            fps_window_ms = car_elapsed_ms;
        }
        fps_window_frames++;
        if((car_elapsed_ms - fps_window_ms) >= 500U)
        {
            uint32 elapsed_ms = car_elapsed_ms - fps_window_ms;
            fps_x10 = (fps_window_frames * 10000U + elapsed_ms / 2U) / elapsed_ms;
            fps_window_frames = 0U;
            fps_window_ms = car_elapsed_ms;
        }
        car_vision_publish_result(&snapshot.result, snapshot.process_us_last, fps_x10);
    }
#endif
}

void core3_main(void)
{
    disable_Watchdog();
    interrupt_global_enable(0);

    cpu_wait_event_ready();

#if !CAR_VISION_ENABLE
    car_nav_encoder_test_init();
    pit_ms_init(CCU60_CH0, CAR_CONTROL_PERIOD_MS);

    while(TRUE)
    {
        if(car_nav_10ms_pending)
        {
            car_nav_10ms_pending--;
            car_nav_encoder_test_10ms();
        }

        system_delay_ms(1U);
    }
#else

    car_nav_init();
    pit_ms_init(CCU60_CH0, CAR_CONTROL_PERIOD_MS);

    while(TRUE)
    {
        km13_vision_command_poll();

        if(car_nav_10ms_pending)
        {
            car_nav_10ms_pending--;
            car_nav_control_10ms();
        }

        system_delay_ms(1U);
    }
#endif
}

#pragma section all restore
