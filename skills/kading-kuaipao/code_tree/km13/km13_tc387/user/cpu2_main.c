/*
 * KM13 TC387 CPU2 voice task entry.
 * Based on the SeekFree TC387 open-source library (GPL-3.0-or-later).
 * Copyright (c) 2022 SEEKFREE Technology.
 */
#include "zf_common_headfile.h"
#include "car_nav.h"
#include "car_voice.h"

#pragma section all "cpu2_dsram"

void core2_main(void)
{
    uint32 last_task_ms = 0;

    disable_Watchdog();
    interrupt_global_enable(0);

    cpu_wait_event_ready();

    car_voice_init();

    while(TRUE)
    {
        if((car_elapsed_ms - last_task_ms) >= CAR_VOICE_TASK_PERIOD_MS)
        {
            last_task_ms = car_elapsed_ms;
            car_voice_task(car_elapsed_ms);
        }

        system_delay_ms(1);
    }
}

#pragma section all restore
