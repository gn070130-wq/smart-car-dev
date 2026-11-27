/*********************************************************************************************************************
 * TC387QP CPU0 task.
 * CAR_VISION_ENABLE selects MT9V03X ownership. IPS200 display output is owned by CPU1 in vision mode.
 ********************************************************************************************************************/
#include "zf_common_headfile.h"
#include "car_config.h"
#include "car_nav.h"
#include "../code/vision_calibration_protocol.h"
#include "../code/vision_calibration_store.h"
#include "../code/vision_shared.h"

#define VISION_DEBUG_UART       (0U)
#define VISION_DEBUG_INTERVAL   (5U)

void core0_main(void)
{
#if CAR_VISION_ENABLE
    uint16 requested_exposure;
#if VISION_DEBUG_UART
    uint32 last_debug_frame = 0U;
#endif
#endif

    clock_init();
    debug_init();

#if !CAR_VISION_ENABLE
    /*
     * Non-vision KM13 modes are owned by CPU3:
     * - CPU3 runs the 10 ms chassis PIT.
     * - CPU3 also owns WiFi in non-vision mode, matching the old KM3 flow.
     * - CPU2 owns the voice UART.
     * CPU0 only brings up the clock/debug layer and releases the multicore
     * startup barrier, so it cannot fight CPU3 for screen or timer hardware.
     */
    cpu_wait_event_ready();
    while(TRUE)
    {
        system_delay_ms(100U);
    }
#else
    vision_shared_init();
    (void)vision_calibration_store_init();

    printf("\r\nTC387QP MT9V03X vision pipeline starting...\r\n");

    while(mt9v03x_init() != 0U)
    {
        printf("MT9V03X init failed, retrying...\r\n");
        system_delay_ms(500U);
    }
    vision_shared_set_exposure_status(MT9V03X_EXP_TIME_DEF, 1U);
    printf("MT9V03X requested: %lu x %lu @ %lu fps, exp=%lu, gain=%lu\r\n",
           (unsigned long)MT9V03X_W,
           (unsigned long)MT9V03X_H,
           (unsigned long)MT9V03X_FPS_DEF,
           (unsigned long)MT9V03X_EXP_TIME_DEF,
           (unsigned long)MT9V03X_GAIN_DEF);
    {
        vision_calibration_t calibration;
        if(vision_calibration_store_get(&calibration) != 0U)
        {
            printf("CAL BOOT valid=1 scale=%lu near=%lu far=%lu seq=%lu slot=%lu\r\n",
                   (unsigned long)calibration.distance_scale_mm_px,
                   (unsigned long)calibration.follow_near_mm,
                   (unsigned long)calibration.follow_far_mm,
                   (unsigned long)calibration.sequence,
                   (unsigned long)calibration.slot);
        }
        else
        {
            printf("CAL BOOT valid=0 reserved_pages=62,63\r\n");
        }
    }

    cpu_wait_event_ready();
    while(TRUE)
    {
        vision_calibration_protocol_poll();

        if(vision_shared_take_exposure_request(&requested_exposure) != 0U)
        {
            uint8 set_failed = mt9v03x_set_exposure_time(requested_exposure);
            vision_shared_set_exposure_status(requested_exposure,
                                              (uint8)((set_failed == 0U) ? 1U : 0U));
        }

#if VISION_DEBUG_UART
        {
            vision_runtime_snapshot_t snapshot;
            mt9v03x_capture_stats_t capture_stats;
            if((vision_shared_read(&snapshot) != 0U)
                    && (snapshot.result.frame_id != last_debug_frame)
                    && ((snapshot.result.frame_id % VISION_DEBUG_INTERVAL) == 0U))
            {
                last_debug_frame = snapshot.result.frame_id;
                mt9v03x_get_capture_stats(&capture_stats);
                printf("tag valid=%lu pred=%lu quad=%lu ex=%ld size=%lu dist=%lu zone=%lu conf=%lu p90=%lu sat=%lu ctr=%lu "
                       "proc=%lu/%luus cam=%lu exp=%lu fail=%lu dmaLost=%lu state=%lu reinit=%lu\r\n",
                       (unsigned long)snapshot.result.valid,
                       (unsigned long)snapshot.result.predicted,
                       (unsigned long)snapshot.result.perspective_corrected,
                       (signed long)snapshot.result.error_x_px,
                       (unsigned long)snapshot.result.size_px,
                       (unsigned long)snapshot.result.distance_mm,
                       (unsigned long)snapshot.result.distance_zone,
                       (unsigned long)snapshot.result.confidence,
                       (unsigned long)snapshot.result.gray_p90,
                       (unsigned long)snapshot.result.saturated_permille,
                       (unsigned long)snapshot.result.tag_contrast,
                       (unsigned long)snapshot.process_us_last,
                       (unsigned long)snapshot.process_us_max,
                       (unsigned long)snapshot.camera_sequence,
                       (unsigned long)snapshot.exposure_applied,
                       (unsigned long)snapshot.exposure_failures,
                       (unsigned long)capture_stats.dma_request_lost_count,
                       (unsigned long)capture_stats.state_error_count,
                       (unsigned long)capture_stats.dma_reinit_count);
            }
        }
#endif
    }
#endif
}
